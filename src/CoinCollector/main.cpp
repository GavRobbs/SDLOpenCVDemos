#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <algorithm>
#include <random>

//For some reason, it really hates it when you allocate these inside a loop
//need to figure out why
cv::Mat temp;
Uint32 coin_add_time{1500};
std::mt19937 gen;
std::uniform_int_distribution<> x_distribution(80, 700);
std::uniform_real_distribution<> speed_multiplier_distribution(1.0, 4.0);
float coin_base_speed = 40.0f;
SDL_Texture * coin_texture;

struct Coin{
    cv::Point2f position;
    float speed_multiplier;

    bool IsIntersecting(cv::Point2f pos){

        //Assuming that each coin is 64 pixels in diameter (so 32 in radius)
        //This is a simple point in circle test
        return cv::norm(pos - position) <= 32.0;
    }

    Coin(float x, float y, float speed_mult): position{cv::Point2f{x,y}}, speed_multiplier{speed_mult}{     
        
    }

    Coin() : position{cv::Point2f{0.0f, 0.0f}}, speed_multiplier{1.0f}{

    }

    void update(float dt){

        position.y += coin_base_speed * speed_multiplier * dt;
    }
};

std::vector<Coin> coins{};

void DrawCoin(const Coin & coin, SDL_Renderer * renderer){
    //I'm using square coins because I can't be pissed to add SDL_image yet

    if(coin.position.y < 32.0f){
        return;
    }

    int left = (int)(coin.position.x - 32.0);
    left = std::max(left, 0);

    int top = (int)(coin.position.y - 32.0);
    top = std::max(top, 0);

    SDL_Rect point_box{left, top, 64, 64};

    SDL_RenderCopy(renderer, coin_texture, nullptr, &point_box);
}


Uint32 addCoinCallback(Uint32 interval, void* param){

    //This callback is called every coin_add_time

    //Spawn up to 3 coins at a time, but at least 1 at a time
    int numCoins = 1 + (x_distribution(gen) % 3);

    for(int i = 0; i < numCoins; i++){

        float random_x = x_distribution(gen);
        float random_y = -10.0;
        coins.push_back(Coin{random_x, random_y, (float)speed_multiplier_distribution(gen)});
    }
    
    return interval;
}

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

cv::Point2f BrightestPixel(const cv::Mat & frame){

    /* See the description in the BrightestPixel project. */

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
    return cv::Point2f{(float)(moments.m10 / moments.m00), (float)(moments.m01 / moments.m00)};
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

    //Generate random numbers with a mersenne twister
    std::random_device rd;
    gen = std::mt19937{rd()};

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0){
        std::cerr << "SDL not initalizing properly" << std::endl;
        return 1;
    }

    window = SDL_CreateWindow("Coin Collector", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if(!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        std::cerr << "SDL image not initalizing properly" << std::endl;
        return 1;
    }

    SDL_Surface* loadedSurface = IMG_Load("./coin.png");
    if(loadedSurface == nullptr){
        std::cerr << "Failed to load coin image" << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        return 1;
    }

    coin_texture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
    SDL_FreeSurface(loadedSurface);

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
    SDL_Texture* frame_texture = nullptr;

    //OpenCV really hates when you make these in your main loop
    cv::Mat frame, gray;

    //Coordinates for the brightest pixel position
    cv::Point2f brightest_pixel_position;

    SDL_TimerID timerID = SDL_AddTimer(coin_add_time, addCoinCallback, nullptr);

    Uint32 last_frame_time = SDL_GetTicks();
    Uint32 current_frame_time = SDL_GetTicks();

    //Time between frames in seconds
    float dt = 0.0f;

    while(!quit){

        last_frame_time = current_frame_time;
        current_frame_time = SDL_GetTicks();
        dt = ((float)(current_frame_time - last_frame_time)) / 1000.0f;
        
        while(SDL_PollEvent(&event)){

            if (event.type == SDL_QUIT) {
                quit = true;
            } 
        }

        webcam_video >> frame;

        //Flips the image horizontally, makes the movements of the light source easier to understand
        cv::flip(frame, frame, 1);

        //Its recommended to do brightest pixel in grayscale
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::GaussianBlur(gray, gray, cv::Size(15, 15), 0);
        cv::threshold(gray, gray, 200, 255, cv::THRESH_BINARY);

        frame_surface = FrameToSurface(frame, renderer);

        brightest_pixel_position = BrightestPixel(gray);

        frame_texture = SDL_CreateTextureFromSurface(renderer, frame_surface);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, frame_texture, NULL, NULL);
        DrawPoint(brightest_pixel_position, renderer, 0, 255, 0);

        /* Update the coin positions, check if they're haven't been touched or fallen offscreen, and draw
        them if they're still valid*/
        for(auto it = coins.begin(); it != coins.end();){
            it->update(dt);
            if(it->IsIntersecting(brightest_pixel_position) || it->position.y >= 600.0f){
                it = coins.erase(it);
            } else{
                DrawCoin(*it, renderer);
                ++it;
            }
        }

        SDL_RenderPresent(renderer);

        SDL_FreeSurface(frame_surface);
        SDL_DestroyTexture(frame_texture);
    }

    webcam_video.release();

    std::cout << "Application finished" << std::endl;
    IMG_Quit();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

}
