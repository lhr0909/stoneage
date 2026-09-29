// SDL2 rendering harness for the VER25 asset port; not the gameplay client.
#include "assets25.h"
#include <SDL.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

template<class T, void(*Destroy)(T*)> using Handle = std::unique_ptr<T, decltype(Destroy)>;
void check(int result) { if (result < 0) throw std::runtime_error(SDL_GetError()); }
int main(int argc, char** argv) try {
    if (argc != 2 && argc != 4) {
        std::cerr << "Usage: stoneage25-assets CLIENT_ROOT [--smoke OUTPUT.bmp]\n"
                     "Left/right: sprite. Up/down: animation. Space: pause. Escape: quit.\n";
        return 2;
    }
    bool smoke = argc == 4 && std::string(argv[2]) == "--smoke";
    if (argc == 4 && !smoke) throw std::runtime_error("Unknown option");
    stoneage::Assets25 assets(argv[1]);
    std::vector<std::uint32_t> ids;
    for (const auto& sprite : assets.sprites()) ids.push_back(sprite.first);
    check(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER));
    struct Quit { ~Quit() { SDL_Quit(); } } quit;
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    Handle<SDL_Window,SDL_DestroyWindow> window(SDL_CreateWindow("Stone Age 2.5 asset preview",
        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,960,720,SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI),SDL_DestroyWindow);
    if (!window) throw std::runtime_error(SDL_GetError());
    Handle<SDL_Renderer,SDL_DestroyRenderer> renderer(SDL_CreateRenderer(window.get(),-1,SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),SDL_DestroyRenderer);
    if (!renderer) renderer.reset(SDL_CreateRenderer(window.get(),-1,SDL_RENDERER_SOFTWARE));
    if (!renderer) throw std::runtime_error(SDL_GetError());
    check(SDL_RenderSetLogicalSize(renderer.get(),640,480));
    struct TextureFrame {
        Handle<SDL_Texture,SDL_DestroyTexture> texture{nullptr,SDL_DestroyTexture};
        SDL_Rect dest{};
    };
    std::vector<TextureFrame> textures;
    std::size_t sprite=0,animation=0,draws=0;
    auto stand = [&]() {
        const auto& animations=assets.sprites().at(ids[sprite]);
        auto found=std::find_if(animations.begin(),animations.end(),[](const auto& a) { return a.action==3 && a.direction==4; });
        animation=found==animations.end() ? 0 : static_cast<std::size_t>(found-animations.begin());
    };
    stand();
    bool running=true,paused=false,dirty=true;
    std::uint64_t elapsed=0;
    auto previous=SDL_GetTicks64();
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running=false;
            if (event.type != SDL_KEYDOWN) continue;
            auto key=event.key.keysym.sym;
            if (key==SDLK_ESCAPE) running=false;
            if (key==SDLK_SPACE) paused=!paused;
            if (key==SDLK_RIGHT || key==SDLK_LEFT) {
                sprite=(sprite+ids.size()+(key==SDLK_RIGHT ? 1 : -1))%ids.size();
                stand(); dirty=true;
            }
            auto count=assets.sprites().at(ids[sprite]).size();
            if (key==SDLK_UP || key==SDLK_DOWN) {
                animation=(animation+count+(key==SDLK_UP ? 1 : -1))%count; dirty=true;
            }
        }
        const auto& anim=assets.sprites().at(ids[sprite])[animation];
        if (dirty) {
            textures.clear(); elapsed=0;
            for (const auto& frame : anim.frames) {
                if (frame.bitmap == UINT32_MAX) { textures.emplace_back(); continue; }
                auto bitmap=assets.image(frame.bitmap);
                auto pixels=assets.rgba(bitmap);
                Handle<SDL_Surface,SDL_FreeSurface> surface(SDL_CreateRGBSurfaceWithFormatFrom(pixels.data(),
                    bitmap.width,bitmap.height,32,bitmap.width*4,SDL_PIXELFORMAT_RGBA32),SDL_FreeSurface);
                if (!surface) throw std::runtime_error(SDL_GetError());
                TextureFrame output;
                output.texture.reset(SDL_CreateTextureFromSurface(renderer.get(),surface.get()));
                if (!output.texture) throw std::runtime_error(SDL_GetError());
                check(SDL_SetTextureBlendMode(output.texture.get(),SDL_BLENDMODE_BLEND));
                output.dest={320+bitmap.x+frame.x,300+bitmap.y+frame.y,static_cast<int>(bitmap.width),static_cast<int>(bitmap.height)};
                textures.push_back(std::move(output));
            }
            std::string title="Stone Age 2.5 asset preview | sprite "+std::to_string(ids[sprite])+
                " | action "+std::to_string(anim.action)+" direction "+std::to_string(anim.direction)+
                " | arrows: browse, space: pause";
            SDL_SetWindowTitle(window.get(),title.c_str());
            dirty=false;
        }
        auto now=SDL_GetTicks64();
        if (!paused) elapsed+=now-previous;
        previous=now;
        auto duration=std::max<std::uint32_t>(anim.duration,1);
        auto frame=(elapsed%duration)*textures.size()/duration;
        check(SDL_SetRenderDrawColor(renderer.get(),40,44,49,255)); check(SDL_RenderClear(renderer.get()));
        check(SDL_SetRenderDrawColor(renderer.get(),62,67,73,255));
        for (int x=0;x<640;x+=32) check(SDL_RenderDrawLine(renderer.get(),x,0,x,480));
        for (int y=12;y<480;y+=32) check(SDL_RenderDrawLine(renderer.get(),0,y,640,y));
        if (textures[frame].texture)
            check(SDL_RenderCopy(renderer.get(),textures[frame].texture.get(),nullptr,&textures[frame].dest));
        if (smoke && draws==30) {
            int w,h; check(SDL_GetRendererOutputSize(renderer.get(),&w,&h));
            Handle<SDL_Surface,SDL_FreeSurface> capture(SDL_CreateRGBSurfaceWithFormat(0,w,h,32,SDL_PIXELFORMAT_RGBA32),SDL_FreeSurface);
            if (!capture) throw std::runtime_error(SDL_GetError());
            check(SDL_RenderReadPixels(renderer.get(),nullptr,SDL_PIXELFORMAT_RGBA32,capture->pixels,capture->pitch));
            check(SDL_SaveBMP(capture.get(),argv[3]));
            std::cout << "Rendered sprite " << ids[sprite] << " with SDL " << SDL_GetCurrentVideoDriver() << " to " << argv[3] << '\n';
            running=false;
        }
        SDL_RenderPresent(renderer.get()); ++draws; SDL_Delay(10);
    }
    return 0;
} catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
