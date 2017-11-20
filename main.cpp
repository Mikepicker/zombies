// Using SDL and standard IO
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <time.h>

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
    float x, y;
    int w, h;
    float vX, vY;
    int scaleX, scaleY;
    int frameX, frameY;
    int animSpeed = 5;
    int speed = 3, jumpSpeed = 10;
    bool animCompleted = false;
    bool shot = false;
    bool stab = false;
    bool alive = true;
    std::string state;
    SDL_Texture* texture;
};

const int ZOMBIE_COUNT = 20;
const int SPAWN_FREQ = 3;
unsigned int lastSpawnTime = 0, currentTime;
int zombieAnimSpeed = 8;
int zombieSpeed = 3;
SDL_Texture* zombieTexture;
struct Zombie {
    float x, y;
    int w = 64;
    int h = 64;
    float vX, vY;
    int dir;
    int frameX, frameY;
    bool animCompleted = false;
    bool attack = false;
    bool alive = false;
    std::string state;
};

const int BULLET_COUNT = 10;
SDL_Texture* bulletTexture;
int bulletSpeed = 20;
struct Bullet {
    float x, y;
    int w, h;
    int dir = 1;
    bool alive = false;
};

struct Background background;
struct Platform platform;
struct Survivor survivor;
struct Bullet bullets[BULLET_COUNT];
struct Zombie zombies[ZOMBIE_COUNT];

// World
struct World {
    float gravity = 0.5f;
};

struct World world;

// Utils
int randInRange(int min, int max) {
    return rand() % (max + 1 - min) + min;
}

void spawnZombie() {
    for (int i = 0; i < ZOMBIE_COUNT; i++) {
        if (!zombies[i].alive) {
            zombies[i].frameX = 0;
            zombies[i].frameY = 0;
            zombies[i].x = randInRange(platform.x, platform.x + platform.w - zombies[i].w);
            zombies[i].dir = survivor.x - zombies[i].x > 0 ? 1 : -1;
            zombies[i].y = 0;
            zombies[i].alive = true;
            zombies[i].state = "state_fall";
            return;
        }
    }
}

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

bool collision(float xA, float xB, float yA, float yB, int wA, int wB, int hA, int hB) {
    if (yA + hA <= yB) {
        return false;
    }

    if (yA >= yB + hB) {
        return false;
    }

    if (xA + wA <= xB) {
        return false;
    }

    if (xA >= xB + wB) {
        return false;
    }

    return true;
}

void hitZombies(struct Bullet *bullet) {

    for (int i = 0; i < ZOMBIE_COUNT; i++) {
        if (zombies[i].alive && collision(bullet->x, zombies[i].x, bullet->y, zombies[i].y, bullet->w, zombies[i].w, bullet->h, zombies[i].h)) {
            zombies[i].vX = bullet->dir * 10;
            zombies[i].state = "state_hit";
            bullet->alive = false;
        }
    }
}

void stabZombies() {

    for (int i = 0; i < ZOMBIE_COUNT; i++) {
        if (zombies[i].alive && collision(survivor.x, zombies[i].x, survivor.y, zombies[i].y, survivor.w, zombies[i].w, survivor.h, zombies[i].h)) {
            zombies[i].vY = -15;
            zombies[i].vX = survivor.scaleX * 5;
            zombies[i].state = "state_hit";
        }
    }
}

bool platformCollision(float x, float y, int w, int h) {
    return y + h > platform.y &&
        x + 48 >= platform.x &&
        x + w - 48 <= platform.x + platform.w;
}

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
    
    // Seed random number generator
    srand(time(NULL));

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
    survivor.vX = 0;
    survivor.vY = 0;
    survivor.scaleX = 1;
    survivor.scaleY = 1;
    survivor.frameX = 0;
    survivor.state = "state_idle";

    if (survivor.texture == NULL) {
        printf("Failed to load survivor texture!\n");
        success = false;
    }

    // Init zombie
    zombieTexture = loadTexture("assets/zombie.png");

    // Init bullet
    bulletTexture = loadTexture("assets/bullet.png");
    for (int i = 0; i < BULLET_COUNT; i++) {
        SDL_QueryTexture(bulletTexture, NULL, NULL, &bullets[i].w, &bullets[i].h);
    }
    
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

    SDL_DestroyTexture(zombieTexture);
    zombieTexture = NULL;

    //Destroy window
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    gRenderer = NULL;
    gWindow = NULL;

    //Quit SDL subsystems
    IMG_Quit();
    SDL_Quit();
}


// Game logic
void update() {

    if (survivor.state != "state_dead") {

        // Apply gravity
        survivor.vY += world.gravity;
        survivor.y += survivor.vY;

        // Motion
        survivor.x += survivor.vX;

        // Platform collision
        if (platformCollision(survivor.x, survivor.y, survivor.w, survivor.h)) {

            survivor.y = platform.y - survivor.h; 
            survivor.vY = 0;

            if (survivor.state == "state_fall" || survivor.state == "state_jump") {
                survivor.frameY = 0;
                survivor.state = "state_idle";
            }

        } else if (survivor.state != "state_jump") {
            survivor.state = "state_fall";
        }

        // Input processing
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        if (survivor.state != "state_shoot" && survivor.state != "state_stab" && survivor.state != "state_fall") {
            
            if (survivor.state == "state_jump") {
                
                if (keys[SDL_SCANCODE_D]) {
                    survivor.vX = survivor.speed;
                    survivor.scaleX = 1;
                } else if (keys[SDL_SCANCODE_A]) {
                    survivor.vX = -survivor.speed;
                    survivor.scaleX = -1;
                } else {
                    survivor.vX = 0;
                }

            } else {
                
                if (keys[SDL_SCANCODE_W]) {
                    survivor.vY = -survivor.jumpSpeed;
                    survivor.frameY = 3;
                    survivor.state = "state_jump";
                } else if (keys[SDL_SCANCODE_J]) {
                    survivor.frameX = 0;
                    survivor.frameY = 2;
                    survivor.vX = 0;
                    survivor.animCompleted = false;
                    survivor.shot = false;
                    survivor.state = "state_shoot";
                } else if (keys[SDL_SCANCODE_K]) {
                    survivor.frameX = 0;
                    survivor.frameY = 5;
                    survivor.vX = 0;
                    survivor.animCompleted = false;
                    survivor.stab = false;
                    survivor.state = "state_stab";
                } else if (keys[SDL_SCANCODE_D]) {
                    survivor.vX = survivor.speed;
                    survivor.scaleX = 1;
                    survivor.frameY = 1;
                    survivor.state = "state_walk";
                } else if (keys[SDL_SCANCODE_A]) {
                    survivor.vX = -survivor.speed;
                    survivor.scaleX = -1;
                    survivor.frameY = 1;
                    survivor.state = "state_walk";
                } else {
                    survivor.frameY = 0;
                    survivor.vX = 0;
                    survivor.state = "state_idle";
                }
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

        if (survivor.state == "state_stab") {

            if (survivor.frameX / survivor.animSpeed == 2 && !survivor.stab) {
                survivor.stab = true;
                stabZombies();
            }

            if (survivor.animCompleted) {
                survivor.frameY = 0;
                survivor.state = "state_idle";
            }
        }
    } else { // state dead
        if (survivor.animCompleted) {
            survivor.alive = false;
        }
    }

    // Zombies
    for (int i = 0; i < ZOMBIE_COUNT; i++) {
        if (zombies[i].alive) {

            // Out of screen
            if (zombies[i].y > SCREEN_HEIGHT) {
                zombies[i].alive = false;
            }

            // Apply gravity
            zombies[i].vY += world.gravity;
            zombies[i].y += zombies[i].vY;

            // Motion
            zombies[i].x += zombies[i].vX;

            // Platform
            if (zombies[i].state == "state_hit") {
                continue;
            }

            if (platformCollision(zombies[i].x, zombies[i].y, zombies[i].w, zombies[i].h)) {
                zombies[i].y = platform.y - zombies[i].h; 
                zombies[i].vY = 0;

                if (zombies[i].state == "state_fall") {
                    zombies[i].state = "state_walk";
                }
            } else {
                zombies[i].vX = 0;
                zombies[i].state = "state_fall";
            }

            if (zombies[i].state == "state_walk") {

                // Move
                zombies[i].vX = zombieSpeed * zombies[i].dir;

                // Attack survivor if colliding
                if (survivor.state != "state_dead" && collision(zombies[i].x, survivor.x, zombies[i].y, survivor.y, zombies[i].w, survivor.w, zombies[i].h, survivor.h)) {
                    zombies[i].frameX = 0;
                    zombies[i].frameY = 2;
                    zombies[i].vX = 0;
                    zombies[i].attack = false;
                    zombies[i].animCompleted = false;
                    zombies[i].state = "state_attack";
                }
            }

            if (zombies[i].state == "state_attack") {

                // Attack survivor
                if (zombies[i].frameX / zombieAnimSpeed == 3 && !zombies[i].attack && survivor.state != "state_jump") {
                    zombies[i].attack = true;
                    survivor.frameX = 0;
                    survivor.frameY = 4;
                    survivor.vX = survivor.vY = 0;
                    survivor.animCompleted = false;
                    survivor.state = "state_dead";
                }
                
                if (zombies[i].animCompleted) {
                    zombies[i].frameX = 0;
                    zombies[i].frameY = 0;
                    zombies[i].state = "state_walk";
                }

            }

            

        }
    }

    // Spawn zombie every N seconds
    currentTime = SDL_GetTicks();
    if (currentTime > lastSpawnTime + SPAWN_FREQ * 1000) {
        spawnZombie();
        lastSpawnTime = currentTime;
    }

    // Bullets
    for (int i = 0; i < BULLET_COUNT; i++) {
        if (bullets[i].alive) {
            bullets[i].x += bulletSpeed * bullets[i].dir;
            
            // Out of screen
            if (bullets[i].x + bullets[i].w < 0 || bullets[i].x > SCREEN_WIDTH) {
                bullets[i].alive = false;
            }

            // Hit zombie
            hitZombies(&bullets[i]);
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
            SDL_Rect dstBullet = { .x = (int)bullets[i].x, .y = (int)bullets[i].y, .w = bullets[i].w, .h = bullets[i].h };
            SDL_RenderCopy(gRenderer, bulletTexture, NULL, &dstBullet);
        }
    }

    // Render survivor
    //printf("%d\n", survivor.alive);
    if (survivor.alive) {
        SDL_Rect srcSurv = { .x = (survivor.frameX / survivor.animSpeed) * survivor.w, .y = survivor.frameY * survivor.h, .w = survivor.w, .h = survivor.h };
        SDL_Rect dstSurv = { .x = (int)survivor.x, .y = (int)survivor.y, .w = survivor.w, .h = survivor.h };
        SDL_RendererFlip flip = survivor.scaleX == 1 ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
        SDL_RenderCopyEx(gRenderer, survivor.texture, &srcSurv, &dstSurv, 0, NULL, flip); 
    }

    // Render zombies
    for (int i = 0; i < ZOMBIE_COUNT; i++) {
        if (zombies[i].alive) {
            SDL_Rect srcZombie = { .x = (zombies[i].frameX / zombieAnimSpeed) * zombies[i].w, .y = zombies[i].frameY * zombies[i].h, .w = zombies[i].w, .h = zombies[i].h };
            SDL_Rect dstZombie = { .x = (int)zombies[i].x, .y = (int)zombies[i].y, .w = zombies[i].w, .h = zombies[i].h };
            SDL_RendererFlip flip = zombies[i].dir == 1 ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
            SDL_RenderCopyEx(gRenderer, zombieTexture, &srcZombie, &dstZombie, 0, NULL, flip);
        }
    }
    
    // Update the screen
    SDL_RenderPresent(gRenderer);

    // Update frames
    survivor.frameX++;
    if (survivor.frameX / survivor.animSpeed >= 4) {
        survivor.animCompleted = true;
        survivor.frameX = 0;
    }

    for (int i = 0; i < ZOMBIE_COUNT; i++) {
        zombies[i].frameX++;
        if (zombies[i].alive && zombies[i].frameX / zombieAnimSpeed >= 4) {
            zombies[i].animCompleted = true;
            zombies[i].frameX = 0;
        }
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
