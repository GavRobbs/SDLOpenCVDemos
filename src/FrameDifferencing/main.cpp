#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <algorithm>
#include <vector>

//For some reason, it really hates it when you allocate these inside a loop
//need to figure out why
cv::Mat temp;
cv::Mat last_frame;
cv::Mat current_frame;
cv::Mat difference_frame;
cv::Mat last_frame_gs;
cv::Mat current_frame_gs;

bool first_frame = true;

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

void DrawContourBounds(SDL_Renderer * renderer, const std::vector<cv::Rect> & boundingRects){

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Set color to red

    for (const auto& rect : boundingRects) {
        SDL_Rect sdlRect = { rect.x, rect.y, rect.width, rect.height };
        SDL_RenderDrawRect(renderer, &sdlRect);
    }

}

int main(int argc, char *argv){

    SDL_Window * window = nullptr;
    SDL_Renderer * renderer = nullptr;

    if(SDL_Init(SDL_INIT_VIDEO) != 0){
        std::cerr << "SDL not initalizing properly" << std::endl;
        return 1;
    }

    window = SDL_CreateWindow("Frame Differencing", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
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

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    std::vector<cv::Rect> boundingRects;
    int detection_threshold = 900;

    int morph_size = 2;
    //A structuring element is kind of like a convolution?
    cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT,
                                                cv::Size(2 * morph_size + 1, 2 * morph_size + 1),
                                                cv::Point(morph_size, morph_size));

    while(!quit){
        while(SDL_PollEvent(&event)){

            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }

        if(first_frame){
            webcam_video >> current_frame;
            cv::cvtColor(current_frame, current_frame_gs, cv::COLOR_BGR2GRAY);
            first_frame = false;
            continue;
        } else{
            //Remember that openCV shallow copies by default
            last_frame = current_frame.clone();
            last_frame_gs = current_frame_gs.clone();

            webcam_video >> current_frame;
            cv::cvtColor(current_frame, current_frame_gs, cv::COLOR_BGR2GRAY);

            difference_frame = current_frame_gs - last_frame_gs;
            cv::medianBlur(difference_frame, difference_frame, 3);
            cv::threshold(difference_frame, difference_frame, 25, 255, cv::THRESH_BINARY);
            cv::medianBlur(difference_frame, difference_frame, 3);
            cv::morphologyEx(difference_frame, difference_frame, cv::MORPH_CLOSE, element);

            contours.clear();
            hierarchy.clear();

            cv::findContours(difference_frame, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            boundingRects.clear();

            for (const auto& contour : contours) {
                auto cnt = cv::boundingRect(contour);
                //Only use contours that are bigger than a certain size to make bounding rects
                if(cnt.width * cnt.height >= detection_threshold){
                    boundingRects.push_back(cnt);
                }
            }
        }

        frame_surface = FrameToSurface(difference_frame, renderer);
        frame_texture = SDL_CreateTextureFromSurface(renderer, frame_surface);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, frame_texture, NULL, NULL);
        DrawContourBounds(renderer, boundingRects);
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
