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

cv::Point BrightestPixelManual(SDL_Surface ** surface){

    /* This is my manual implementation of brightest pixel, just to compare to what I get from using openCV*/

    SDL_LockSurface(*surface);

    Uint8 * pixelData = (Uint8*)((*surface)->pixels);
    int brightest_pixel_value = 0;
    int bp_x = 0;
    int bp_y = 0;

    for(int j = 0; j < 600; j++){
        for(int i = 0; i < 800 * 3; i += 3){
            int r = static_cast<int>(pixelData[(j * 800 * 3) + i]);
            int g = static_cast<int>(pixelData[(j * 800 * 3) + i + 1]);
            int b = static_cast<int>(pixelData[(j * 800 * 3) + i + 2]);

            int pv = r + g + b;
            if(pv >= brightest_pixel_value){
                brightest_pixel_value = pv;
                bp_x = i / 3;
                bp_y = j;
            }
        }
    }

    SDL_UnlockSurface(*surface);
    return cv::Point(bp_x, bp_y);
}

cv::Point BrightestPixelOpenCV(const cv::Mat & frame){

    /* We find the shapes of the contours of the image, then get the centroid from the moments, 
    and that should be our brightest point. It actually works very well. Could definitely use this for motion
    tracking in a dark room.*/

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(frame, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    double area_max = 0;
    std::vector<cv::Point> maxContour;
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area > area_max) {
            area_max = area;
            maxContour = contour;
        }
    }

    cv::Moments moments = cv::moments(maxContour, false);
    //Return the centroid
    return cv::Point{(int)(moments.m10 / moments.m00), (int)(moments.m01 / moments.m00)};
}

void DrawPoint(const cv::Point & point, SDL_Renderer * renderer, Uint8 r = 255, Uint8 g = 0, Uint8 b = 0, int width_px = 20, int height_px = 20){

    //Generate the topleft coords to draw the rect
    int left = point.x - width_px/2;
    left = std::max(left, 0);

    int top = point.y - height_px/2;
    top = std::max(top, 0);

    SDL_Rect point_box{left, top, width_px, height_px};
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderFillRect(renderer, &point_box);

}

int main(int argc, char *argv){

    SDL_Window * window = nullptr;
    SDL_Renderer * renderer = nullptr;

    if(SDL_Init(SDL_INIT_VIDEO) != 0){
        std::cerr << "SDL not initalizing properly" << std::endl;
        return 1;
    }

    window = SDL_CreateWindow("Brightest Pixel Tracking", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
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

    //The color frame surface
    SDL_Surface* frame_surface = nullptr;

    //The grayscale frame surface
    SDL_Surface * gs_frame_surface = nullptr;

    SDL_Texture* frame_texture = nullptr;

    //OpenCV really hates when you make these in your main loop
    cv::Mat frame, gray;

    //The point from my manual calculation and the point openCV worked out
    cv::Point brightest_pixel_position_manual;
    cv::Point brightest_pixel_position_ocv;

    while(!quit){
        while(SDL_PollEvent(&event)){

            if (event.type == SDL_QUIT) {
                quit = true;
            } 
        }

        webcam_video >> frame;

        //Its recommended to do brightest pixel in grayscale
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::GaussianBlur(gray, gray, cv::Size(15, 15), 0);
        cv::threshold(gray, gray, 200, 255, cv::THRESH_BINARY);

        gs_frame_surface = FrameToSurface(gray, renderer);
        frame_surface = FrameToSurface(frame, renderer);

        brightest_pixel_position_manual = BrightestPixelManual(&gs_frame_surface);
        brightest_pixel_position_ocv = BrightestPixelOpenCV(gray);

        frame_texture = SDL_CreateTextureFromSurface(renderer, frame_surface);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, frame_texture, NULL, NULL);
        DrawPoint(brightest_pixel_position_manual, renderer);
        DrawPoint(brightest_pixel_position_ocv, renderer, 0, 255, 0);
        SDL_RenderPresent(renderer);

        SDL_FreeSurface(frame_surface);
        SDL_FreeSurface(gs_frame_surface);
        SDL_DestroyTexture(frame_texture);
    }

    webcam_video.release();

    std::cout << "Application finished" << std::endl;

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

}
