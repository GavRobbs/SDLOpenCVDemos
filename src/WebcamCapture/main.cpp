#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <algorithm>

//For some reason, it really hates it when you allocate these inside a loop
//need to figure out why
cv::Mat temp;

SDL_Surface * FrameToSurface(const cv::Mat &frame, SDL_Renderer * renderer){
    cv::cvtColor(frame, temp, cv::COLOR_BGR2RGB);

    SDL_Surface * surface = SDL_CreateRGBSurfaceFrom( 
    (void*)temp.data, 
    temp.cols,
    temp.rows,
    24,
    temp.step,
    0x000000FF,
    0x0000FF00,
    0x00FF0000,
    0x00000000);

    return surface;

}

/* I manually implemented these effects myself although openCV offers them.*/

void GrayscaleEffect(SDL_Surface ** surface){
    void * pixels;

    SDL_LockSurface(*surface);

    Uint8 * pixelData = (Uint8*)((*surface)->pixels);

    for(size_t i = 0; i < 800 * 600 * 3; i += 3){
        //This is called the NTSC formula
        //The larger coefficient for green is because the human eye is most responsive
        //to light in that wavelength
        Uint8 color_gs = static_cast<Uint8>(0.299f * (float)pixelData[i] + 0.587f * (float)pixelData[i + 1] + 0.114f * (float)pixelData[i + 2]);
        pixelData[i] = color_gs;
        pixelData[i + 1] = color_gs;
        pixelData[i + 2] = color_gs;
    }

    SDL_UnlockSurface(*surface);
}

void InverseGrayscaleEffect(SDL_Surface ** surface){
    void * pixels;

    SDL_LockSurface(*surface);

    Uint8 * pixelData = (Uint8*)((*surface)->pixels);

    for(size_t i = 0; i < 800 * 600 * 3; i += 3){
        Uint8 color_gs = static_cast<Uint8>(0.299f * (float)pixelData[i] + 0.587f * (float)pixelData[i + 1] + 0.114f * (float)pixelData[i + 2]);
        pixelData[i] = 255-color_gs;
        pixelData[i + 1] = 255-color_gs;
        pixelData[i + 2] = 255-color_gs;
    }

    SDL_UnlockSurface(*surface);
}

void SepiaEffect(SDL_Surface ** surface){
    void * pixels;

    SDL_LockSurface(*surface);

    Uint8 * pixelData = (Uint8*)((*surface)->pixels);

    for(size_t i = 0; i < 800 * 600 * 3; i += 3){
        int sepia_red = static_cast<int>(0.393f * (float)pixelData[i] + 0.769f * (float)pixelData[i + 1] + 0.189f * (float)pixelData[i + 2]);
        int sepia_green = static_cast<int>(0.349f * (float)pixelData[i] + 0.686f * (float)pixelData[i + 1] + 0.168f * (float)pixelData[i + 2]);
        int sepia_blue = static_cast<int>(0.272f * (float)pixelData[i] + 0.534f * (float)pixelData[i + 1] + 0.131f * (float)pixelData[i + 2]);

        //If you notice, the coefficients don't add up to 1,
        //so we have to clamp
        sepia_red = std::min(sepia_red, 255);
        sepia_green = std::min(sepia_green, 255);
        sepia_blue = std::min(sepia_blue, 255);

        pixelData[i] = (Uint8)sepia_red;
        pixelData[i + 1] = (Uint8)sepia_green;
        pixelData[i + 2] = (Uint8)sepia_blue;
    }

    SDL_UnlockSurface(*surface);
}

int main(int argc, char *argv){

    SDL_Window * window = nullptr;
    SDL_Renderer * renderer = nullptr;

    if(SDL_Init(SDL_INIT_VIDEO) != 0){
        std::cerr << "SDL not initalizing properly" << std::endl;
        return 1;
    }

    window = SDL_CreateWindow("WebcamCapture", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    bool quit = false;
    SDL_Event event;

    cv::VideoCapture webcam_video{0};

    if(!webcam_video.isOpened()){
        std::cout << "Error opening webcam" << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    webcam_video.set(cv::CAP_PROP_FRAME_WIDTH, 800);
    webcam_video.set(cv::CAP_PROP_FRAME_HEIGHT, 600);

    std::cout << "Webcam opened!" << std::endl;

    SDL_Surface* frame_surface = nullptr;
    SDL_Texture* frame_texture = nullptr;
    cv::Mat frame;

    int filter = 0;


    while(!quit){
        while(SDL_PollEvent(&event)){

            if (event.type == SDL_QUIT) {
                quit = true;
            } else if(event.type == SDL_KEYDOWN){
                if(event.key.keysym.sym == SDLK_1){
                    //Press 1 for a normal image
                    filter = 0;
                } else if(event.key.keysym.sym == SDLK_2){
                    //Press 2 for grayscale
                    filter = 1;
                } else if(event.key.keysym.sym == SDLK_3){
                    //Press 3 for sepia
                    filter = 2;
                } else if(event.key.keysym.sym == SDLK_4){
                    //Press 4 for inverse grayscale (whitescale?)
                    filter = 3;
                }
            }

        }

        webcam_video >> frame;

        if(frame.empty()) {
            std::cerr << "Captured empty frame" << std::endl;
            break;
        } 

        frame_surface = FrameToSurface(frame, renderer);

        if(filter == 1){
            GrayscaleEffect(&frame_surface);
        } else if(filter == 2){
            SepiaEffect(&frame_surface);
        } else if(filter == 3){
            InverseGrayscaleEffect(&frame_surface);
        }
        
        frame_texture = SDL_CreateTextureFromSurface(renderer, frame_surface);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, frame_texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        SDL_FreeSurface(frame_surface);
        SDL_DestroyTexture(frame_texture);
    }

    webcam_video.release();

    std::cout << "Application finished" << std::endl;

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

}
