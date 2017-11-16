// Using SDL and standard IO
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <string>

// Screen dimension constants
const int SCREEN_WIDTH = 512;
const int SCREEN_HEIGHT = 400;

// Starts up SDL and creates window
bool init();

// Loads media
bool loadMedia();

// Frees media and shuts down SDL
void close();

// Loads individual image as texture
SDL_Texture* loadTexture(std::string path);

// The window we'll be rendering to
SDL_Window* gWindow = NULL;
	
// Renderer
SDL_Renderer* gRenderer = NULL;

// Game objects
struct Background {
    int x, y, w, h;
    SDL_Texture* texture;
};

struct Platform {
    int x, y, w, h;
    SDL_Texture* texture;
};

struct Survivor {
    int x, y, w, h;
    int scaleX, scaleY;
    int frameX, frameY;
    int animSpeed = 6;
    int speed = 3;
    bool animCompleted = false;
    bool shot = false;
    std::string state;
    SDL_Texture* texture;
};

const int BULLET_COUNT = 10;
SDL_Texture* bulletTexture;
int bulletWidth, bulletHeight;
int bulletSpeed = 20;
struct Bullet {
    int x, y;
    int dir = 1;
    bool alive = false;
};

struct Background background;
struct Platform platform;
struct Survivor survivor;
struct Bullet bullets[BULLET_COUNT];

// World
struct World {
    int gravity = 10;
};

struct World world;

bool init() {

	// Initialization flag
	bool success = true;

	// Initialize SDL
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		success = false;
	}
	else {
		// Create window
		gWindow = SDL_CreateWindow("Zombies", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (gWindow == NULL) {
			printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
			success = false;
		}
		else {
            // Create renderer
            gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            if (gRenderer == NULL) {
                printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
                success = false;
            } else {

                // Init renderer color
                SDL_SetRenderDrawColor(gRenderer, 0xdf, 0xda, 0xd2, 0xff);

                // Init png loading
                int imgFlags = IMG_INIT_PNG;
                if (!(IMG_Init(imgFlags) & imgFlags)) {
                    printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
                    success = false;
                }
            }
		}
	}

	return success;
}

bool loadMedia() {

    // Loading success flag
    bool success = true;

    // Init background
    background.texture = loadTexture("assets/background.png");
    SDL_QueryTexture(background.texture, NULL, NULL, &background.w, &background.h);
    background.x = 0;
    background.y = SCREEN_HEIGHT-background.h;

    if (background.texture == NULL) {
        printf("Failed to load background!\n");
        success = false;
    }

    // Init platform
    platform.texture = loadTexture("assets/platform.png");
    SDL_QueryTexture(platform.texture, NULL, NULL, &platform.w, &platform.h);
    platform.x = (SCREEN_WIDTH / 2) - (platform.w / 2);
    platform.y = 300;

    if (platform.texture == NULL) {
        printf("Failed to load platform!\n");
        success = false;
    }

    // Init survivor
    survivor.texture = loadTexture("assets/survivor.png");
    survivor.x = 200;
    survivor.y = 100;
    survivor.w = 64;
    survivor.h = 64;
    survivor.scaleX = 1;
    survivor.scaleY = 1;
    survivor.frameX = 0;
    survivor.state = "state_idle";

    if (survivor.texture == NULL) {
        printf("Failed to load survivor texture!\n");
        success = false;
    }

    // Init bullet
    bulletTexture = loadTexture("assets/bullet.png");
    SDL_QueryTexture(bulletTexture, NULL, NULL, &bulletWidth, &bulletHeight);

    return success;
}

SDL_Texture* loadTexture(std::string path) {
    
    SDL_Texture* newTexture = NULL;

    // Load image
    SDL_Surface* loadedSurface = IMG_Load(path.c_str());
    if (loadedSurface == NULL) {
        printf("Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
    } else {
        
        newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
        if (newTexture == NULL) {
            printf("Unable to create texture from %s! SDL_Error: %s\n", path.c_str(), SDL_GetError());
        }

        SDL_FreeSurface(loadedSurface);
    }

    return newTexture;
}

void close() {

    //Free loaded image
    SDL_DestroyTexture(background.texture);
    background.texture = NULL;

    SDL_DestroyTexture(platform.texture);
    platform.texture = NULL;

    SDL_DestroyTexture(survivor.texture);
    survivor.texture = NULL;

    SDL_DestroyTexture(bulletTexture);
    bulletTexture = NULL;

    //Destroy window
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    gRenderer = NULL;
    gWindow = NULL;

    //Quit SDL subsystems
    IMG_Quit();
    SDL_Quit();
}

// Utils
void shootBullet() {

    for (int i = 0; i < BULLET_COUNT; i++) {
        if (!bullets[i].alive) {
            if (survivor.scaleX == 1) {
                bullets[i].x = survivor.x + 48;
            } else {
                bullets[i].x = survivor.x + 16;
            }

            bullets[i].y = survivor.y + 32;
            bullets[i].dir = survivor.scaleX;
            bullets[i].alive = true;
            return;
        }
    }
}

// Game logic
void update() {

    // Physics
    survivor.y += world.gravity;

    // Platform collision
    if (survivor.y + survivor.h > platform.y &&
        survivor.x + 48 >= platform.x &&
        survivor.x + survivor.w - 48 <= platform.x + platform.w &&
        survivor.y + (survivor.h/2) < platform.y) {
        survivor.y = platform.y - survivor.h; 
    }

    // Input processing
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    if (survivor.state != "state_shoot") {
        if (keys[SDL_SCANCODE_D]) {
            survivor.x += survivor.speed;
            survivor.scaleX = 1;
            survivor.frameY = 1;
            survivor.state = "state_walk";
        } else if (keys[SDL_SCANCODE_A]) {
            survivor.x -= survivor.speed;
            survivor.scaleX = -1;
            survivor.frameY = 1;
            survivor.state = "state_walk";
        } else if (keys[SDL_SCANCODE_J]) {
            survivor.frameX = 0;
            survivor.frameY = 2;
            survivor.animCompleted = false;
            survivor.shot = false;
            survivor.state = "state_shoot";
        } else {
            survivor.frameX = 0;
            survivor.frameY = 0;
            survivor.state = "state_idle";
        }
    }

    // Player states
    if (survivor.state == "state_shoot") {

        if (survivor.frameX / survivor.animSpeed == 2 && !survivor.shot) {
            survivor.shot = true;
            shootBullet();
        }

        if (survivor.animCompleted) {
            survivor.frameY = 0;
            survivor.state = "state_idle";
        }
    }

    // Bullets
    for (int i = 0; i < BULLET_COUNT; i++) {
        if (bullets[i].alive) {
            bullets[i].x += bulletSpeed * bullets[i].dir;

            if (bullets[i].x + bulletWidth < 0 || bullets[i].x > SCREEN_WIDTH) {
                bullets[i].alive = false;
            }
        }
    }
}

void render() {

    // Clear screen
    SDL_RenderClear(gRenderer);

    // Render bg
    SDL_Rect dst = { .x = background.x, .y = background.y, .w = background.w, .h = background.h };
    SDL_RenderCopy(gRenderer, background.texture, NULL, &dst);

    // Render platform
    SDL_Rect dstPlatf = { .x = platform.x, .y = platform.y, .w = platform.w, .h = platform.h };
    SDL_RenderCopy(gRenderer, platform.texture, NULL, &dstPlatf);

    // Render bullets
    for (int i = 0; i < BULLET_COUNT; i++) {
        if (bullets[i].alive) {
            SDL_Rect dstBullet = { .x = bullets[i].x, .y = bullets[i].y, .w = bulletWidth, .h = bulletHeight };
            SDL_RenderCopy(gRenderer, bulletTexture, NULL, &dstBullet);
        }
    }

    // Render survivor
    SDL_Rect srcSurv = { .x = (survivor.frameX / survivor.animSpeed) * survivor.w, .y = survivor.frameY * survivor.h, .w = survivor.w, .h = survivor.h };
    SDL_Rect dstSurv = { .x = survivor.x, .y = survivor.y, .w = survivor.w, .h = survivor.h };
    SDL_RendererFlip flip = survivor.scaleX == 1 ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
    SDL_RenderCopyEx(gRenderer, survivor.texture, &srcSurv, &dstSurv, 0, NULL, flip); 
    
    // Update the screen
    SDL_RenderPresent(gRenderer);

    survivor.frameX++;
    if (survivor.frameX / survivor.animSpeed >= 4) {
        survivor.animCompleted = true;
        survivor.frameX = 0;
    }

}

int main(int argc, char* args[]) {

	//Start up SDL and create window
	if(!init()) {
		printf("Failed to initialize!\n");
	}
	else {
		// Load media
		if(!loadMedia()) {
			printf("Failed to load media!\n");
		}
		else {

            // Main loop flag
            bool quit = false;

            // Event handler
            SDL_Event e;

            // While application is running
            while(!quit) {

                // Handle events on queue
                while(SDL_PollEvent(&e) != 0) {

                    // User requests quit
                    if(e.type == SDL_QUIT) {
                        quit = true;
                    }
                }

                // Update game
                update();

                
                // Render game
                render();

            }
        }
	}

	//Free resources and close SDL
	close();

	return 0;
}
