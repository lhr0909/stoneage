#include "codec_portability.h"
#include "autil.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <string>
#include <vector>
#include <iostream>

void lssproto_Send(int fd, char *message) {
    std::string data(message); data += '\n';
    for (size_t sent=0; sent<data.size();) {
        ssize_t n = send(fd, data.data()+sent, data.size()-sent, 0);
        if (n<=0) throw std::runtime_error("send failed");
        sent += size_t(n);
    }
}
std::string pending;
int receive(int fd) {
    while(pending.find('\n') == std::string::npos) {
        pollfd p{fd, POLLIN, 0};
        if (poll(&p, 1, 10000)<=0) throw std::runtime_error("response timeout");
        char bytes[4096]; ssize_t n=recv(fd, bytes, sizeof(bytes), 0);
        if(n<=0) throw std::runtime_error("server disconnected");
        pending.append(bytes, size_t(n));
        if(pending.size()>65000) throw std::runtime_error("packet too long");
    }
    size_t end=pending.find('\n'); std::string line=pending.substr(0,end+1); pending.erase(0,end+1);
    char decoded[65500]{};
    util_DecodeMessage(decoded,line.data());
    util_DiscardMessage(); util_SplitMessage(decoded,const_cast<char*>(";"));
    int function=0, fields=0;
    if(!util_GetFunctionFromSlice(&function,&fields)) throw std::runtime_error("invalid packet");
    std::cout << "Received function " << function << ", fields " << fields << std::endl;
    if(function==72 || function==74 || function==78 || function==80 || function==82) {
        int expectedFields = function==72 ? 2 : 3;
        if(fields!=expectedFields) throw std::runtime_error("unexpected response fields");
        int checksum=0, receivedChecksum=0; char value[65500]{};
        for(int i=2; i<fields+1; ++i) checksum+=util_destring(i,value);
        util_deint(fields+1,&receivedChecksum);
        if(checksum!=receivedChecksum) throw std::runtime_error("response checksum mismatch");
    }
    return function;
}
void selfTest() {
    util_Init(); strcpy(PersonalKey,"cary");
    for(int seed=0; seed<100; ++seed) {
        srand(seed);
        char raw[]="&;71;abc;def;#;", encoded[65500]{}, decoded[65500]{};
        util_EncodeMessage(encoded,raw); util_DecodeMessage(decoded,encoded);
        if(strcmp(raw,decoded)) throw std::runtime_error("packet roundtrip mismatch");
    }
    for(int value : {0,1,-1,2147483647,(-2147483647-1)}) {
        char field[16384]{}, valueOut[65500]{}; int decoded=0;
        util_mkint(field,value); util_DiscardMessage();
        std::string packet="&;71"+std::string(field)+";#;";
        util_SplitMessage(packet.data(),const_cast<char*>(";"));
        util_deint(2,&decoded);
        if(decoded!=value) throw std::runtime_error("integer roundtrip mismatch");
    }
    char malformed[]="&;71;missing-end;";
    util_DiscardMessage(); util_SplitMessage(malformed,const_cast<char*>(";"));
    int function=0, fields=0;
    if(util_GetFunctionFromSlice(&function,&fields)) throw std::runtime_error("accepted missing terminator");
    char empty[]="", output[65500]{}; util_DecodeMessage(output,empty);
    util_Release(); std::cout << "Codec checks passed" << std::endl;
}
int main(int argc,char **argv) {
    try {
        if(argc==2 && !strcmp(argv[1],"--self-test")) { selfTest(); return 0; }
        if(argc!=3 && argc!=4) throw std::runtime_error("Usage: stoneage25-probe ACCOUNT PASSWORD [CHARACTER]");
        if(strlen(argv[1])>27 || strlen(argv[2])>31) throw std::runtime_error("credentials exceed legacy limits");
        if(argc==4 && (strlen(argv[3])>31 || strchr(argv[3],'|') || strchr(argv[3],'\\'))) throw std::runtime_error("invalid character name");
        int fd=socket(AF_INET,SOCK_STREAM,0);
        sockaddr_in addr{}; addr.sin_family=AF_INET; addr.sin_port=htons(9065); inet_pton(AF_INET,"127.0.0.1",&addr.sin_addr);
        if(connect(fd,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))) throw std::runtime_error("cannot connect to localhost:9065");
        char greeting[256]{};
        for(size_t n=0; n<sizeof(greeting)-1; ++n) {
            pollfd p{fd,POLLIN,0};
            if(poll(&p,1,10000)<=0 || recv(fd,greeting+n,1,0)!=1) throw std::runtime_error("greeting failed");
            if(!greeting[n]) break;
        }
        std::cout << "Server greeting: " << greeting << std::endl;
        if(greeting[0]!='L') throw std::runtime_error("expected 2.5 server greeting L");
        util_Init(); strcpy(PersonalKey,_DEFAULT_PKEY);
        char buffer[16384]{}; int checksum=util_mkstring(buffer,argv[1])+util_mkstring(buffer,argv[2]);
        util_mkint(buffer,checksum); util_SendMesg(fd,71,buffer);
        snprintf(PersonalKey,32,"%s%s",argv[1],_RUNNING_KEY);
        int function=receive(fd); char result[65500]{};
        if(function!=72) throw std::runtime_error("expected ClientLogin response");
        util_destring(2,result); std::cout << "Login: " << result << std::endl;
        if(strcmp(result,"ok")) throw std::runtime_error("login rejected");
        snprintf(PersonalKey,32,"%s%s",argv[1],_RUNNING_KEY);
        buffer[0]=0; util_mkint(buffer,0); util_SendMesg(fd,79,buffer);
        function=receive(fd);
        if(function!=80) throw std::runtime_error("expected CharList response");
        util_destring(2,result); std::cout << "Character list: " << result << std::endl;
        if(strcmp(result,"successful")) throw std::runtime_error("character list rejected");
        util_destring(3,result); std::cout << "Characters: " << result << std::endl;
        if(argc==4) {
            std::string existing(result);
            if(existing.empty()) {
                buffer[0]=0; checksum=util_mkint(buffer,0);
                checksum+=util_mkstring(buffer,argv[3]);
                for(int value : {100000,30000,5,5,5,5,10,0,0,0,0}) checksum+=util_mkint(buffer,value);
                util_mkint(buffer,checksum); util_SendMesg(fd,73,buffer);
                if(receive(fd)!=74) throw std::runtime_error("expected CreateNewChar response");
                util_destring(2,result); std::cout << "Create character: " << result << std::endl;
                if(strcmp(result,"successful")) throw std::runtime_error("character creation rejected");
            } else if(existing.find(argv[3])==std::string::npos) {
                throw std::runtime_error("account already has different characters; refusing to overwrite");
            }
            buffer[0]=0; checksum=util_mkstring(buffer,argv[3]);
            util_mkint(buffer,checksum); util_SendMesg(fd,77,buffer);
            int count=0;
            do { function=receive(fd); } while(function!=78 && ++count<200);
            if(function!=78) throw std::runtime_error("expected CharLogin response");
            util_destring(2,result); std::cout << "Enter world: " << result << std::endl;
            if(strcmp(result,"successful")) throw std::runtime_error("character login rejected");
            buffer[0]=0; util_mkint(buffer,0); util_mkint(buffer,0); util_SendMesg(fd,81,buffer);
            count=0;
            do { function=receive(fd); } while(function!=82 && ++count<200);
            if(function!=82) throw std::runtime_error("expected CharLogout response");
            util_destring(2,result); std::cout << "Save/logout: " << result << std::endl;
            if(strcmp(result,"successful")) throw std::runtime_error("logout rejected");
        }
        close(fd); util_Release(); return 0;
    } catch(const std::exception &e) { std::cerr << e.what() << std::endl; return 1; }
}
