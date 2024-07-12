#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define TRUE 1
#define FALSE 0

#define PLAYER_SPEED 5
#define PLAYER_BULLET_SPEED 20
#define PLAYER_BULLET_COUNT 10
#define ENEMY_SPEED 3
#define ENEMY_COUNT 10
#define ENEMY_BULLET_COUNT 10
#define ENEMY_BULLET_SPEED 5

typedef struct App {
    SDL_Renderer *renderer;
    SDL_Window *window;
    int up;
    int down;
    int left;
    int right;
    int fire;
} App;

typedef struct Entity {
    int x;
    int y;
    int w;
    int h;
    SDL_Texture *texture;
    int health; // 0 = inactivo, 1 = activo
    int fireCooldown;
} Entity;

int game_is_running;

App app;
Entity *player, *bullet, bulletList[PLAYER_BULLET_COUNT], *enemy, enemyList[ENEMY_COUNT], *enemyBullet, enemyBulletList[ENEMY_BULLET_COUNT];


void destroy_window() {
    free(player);
    SDL_DestroyRenderer(app.renderer);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
}


void doKeyDown(SDL_KeyboardEvent *event) {
    if (event->repeat == 0) {
        if (event->keysym.scancode == SDL_SCANCODE_W) {
            app.up = 1;
        }
        if (event->keysym.scancode == SDL_SCANCODE_S) {
            app.down = 1;
        }
        if (event->keysym.scancode == SDL_SCANCODE_A) {
            app.left = 1;
        }
        if (event->keysym.scancode == SDL_SCANCODE_D) {
            app.right = 1;
        }
        if (event->keysym.scancode == SDL_SCANCODE_SPACE) {
            app.fire = 1;
        }
    }
}
void doKeyUp(SDL_KeyboardEvent *event) {
    if (event->repeat == 0) {
        if (event->keysym.scancode == SDL_SCANCODE_W) {
            app.up = 0;
        }
        if (event->keysym.scancode == SDL_SCANCODE_S) {
            app.down = 0;
        }
        if (event->keysym.scancode == SDL_SCANCODE_A) {
            app.left = 0;
        }
        if (event->keysym.scancode == SDL_SCANCODE_D) {
            app.right = 0;
        }
        if (event->keysym.scancode == SDL_SCANCODE_SPACE) {
            app.fire = 0;
        }
    }
}


void doInput(void) {
    SDL_Event event;
    SDL_PollEvent(&event);

    switch(event.type) {
        case SDL_QUIT:
            game_is_running = FALSE;
            break;

        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE)
                game_is_running = FALSE;

            doKeyDown(&event.key); // manejar eventos de tecla presionada
            break;
        
        case SDL_KEYUP:
            doKeyUp(&event.key); // manejar eventos de tecla liberada
            break;
        
        default:
            break;
    }
}


SDL_Texture *loadTexture(char *filename) {
    SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "Loading %s", filename);
    SDL_Texture *texture;
    texture = IMG_LoadTexture(app.renderer, filename);

    return texture;
}


void render(SDL_Texture *texture, int x, int y) {
    SDL_Rect dest;
    dest.x = x;
    dest.y = y;

    SDL_QueryTexture(texture, NULL, NULL, &dest.w, &dest.h);
    SDL_RenderCopy(app.renderer, texture, NULL, &dest);
}


void initStage() {
    // inicializar jugador
    player = malloc(sizeof(Entity));
    memset(player, 0, sizeof(Entity));
    player->x = 100;
    player->y = (SCREEN_HEIGHT / 2) - 18;
    player->w = 45;
    player->h = 45;
    player->health = 1;
    player->fireCooldown = 0;
    player->texture = loadTexture("./sprites/player.png");

    // inicializar la lista de balas del jugador
    for (int i = 0; i < PLAYER_BULLET_COUNT; i++) {
        bulletList[i].x -= 400; // fuera de la pantalla
        bulletList[i].y = 0;
        bulletList[i].w = 10;
        bulletList[i].h = 10;
        bulletList[i].health = 0;
        bulletList[i].fireCooldown = 0;
        bulletList[i].texture = loadTexture("./sprites/playerBullet.png");
    }

    // inicializar tanda de 5 enemigos
    for (int i = 0; i < ENEMY_COUNT; i++) {
        enemyList[i].x = SCREEN_WIDTH + (i + 1) * 200; // fuera de la pantalla en la primera iteracion
        enemyList[i].y = (rand() % SCREEN_HEIGHT) - 100;
        enemyList[i].w = 45;
        enemyList[i].h = 45;
        enemyList[i].health = 1;
        enemyList[i].fireCooldown = 0;
        enemyList[i].texture = loadTexture("./sprites/enemy.png");
    }

    // inicializar la lista de balas de los enemigos
    for (int i = 0; i < ENEMY_BULLET_COUNT; i++) {
        enemyBulletList[i].x -= 400; // fuera de la pantalla
        enemyBulletList[i].y = 0;
        enemyBulletList[i].w = 10;
        enemyBulletList[i].h = 10;
        enemyBulletList[i].health = 0;
        enemyBulletList[i].fireCooldown = 0;
        enemyBulletList[i].texture = loadTexture("./sprites/alienBullet.png");
    }
}


int checkEntityCollision(Entity *entiy_a, Entity *entiy_b) {
    // colisiones entre jugador y enemigo
    SDL_Rect entiy_a_Rect = { entiy_a->x, entiy_a->y, entiy_a->w, entiy_a->h };
    SDL_Rect entiy_b_Rect = { entiy_b->x, entiy_b->y, entiy_b->w, entiy_b->h };

    if (SDL_HasIntersection(&entiy_a_Rect, &entiy_b_Rect)) {
        return TRUE; // colision
    } 
    return FALSE; // sin colision
}


void enemyLogic() {
    for (int i = 0; i < ENEMY_COUNT; i++) { // recorrer la lista de 5 enemigos

        // mover enemigos
        enemy = &enemyList[i];
        if (enemy->x > 0) { // mientras no llegue al borde izquierdo
            enemy->x -= ENEMY_SPEED; // incrementar x
        }
        else {
            enemy->x = SCREEN_WIDTH + (i + 1) * 200; // reiniciar posicion x
            enemyList[i].y = rand() % SCREEN_HEIGHT; // reiniciar posicion y
            if (enemyList[i].y <= 0) { // si la posicion y es menor o igual a 0
                enemyList[i].y += 100;
            }
            else if (enemyList[i].y >= SCREEN_HEIGHT - 45) {
                enemyList[i].y -= 100;
            }
        }

        // control de disparo
        // si el enemigo esta activo y el cooldown de disparo es mayor al valor aleatorio
        if (enemy->health == 1 && enemy->fireCooldown > rand() % 300 + 100) {
            for (int j = 0; j < ENEMY_BULLET_COUNT; j++) { // recorrer la lista de balas de los enemigos
                if (enemyBulletList[j].health == 0) { 
                    enemyBulletList[j].x = enemy->x - 10; // ajustar la posicion de la bala
                    enemyBulletList[j].y = enemy->y + 18; // ajustar la posicion de la bala
                    enemyBulletList[j].health = 1;
                    break;
                }
            }
            enemy->fireCooldown = 0; // Reinicia el cooldown de disparo
        }
        enemy->fireCooldown++; // Incrementa el cooldown de disparo

        render(enemy->texture, enemy->x, enemy->y);
    }
}


void enemyBulletLogic() {
    for (int i = 0; i < ENEMY_BULLET_COUNT; i++) {
        enemyBullet = &enemyBulletList[i];
        if (enemyBullet->health == 1) {
            enemyBullet->x -= ENEMY_BULLET_SPEED;
            // si la bala de un enemigo colisiona con el jugador
            if (checkEntityCollision(enemyBullet, player) && player->health == 1) {
                enemyBullet->health = 0;
                enemyBullet->x = SCREEN_WIDTH + 400; // fuera de la pantalla
                enemyBullet->y = 0;
                player->health = 0; // desactivar jugador
                player->x = SCREEN_WIDTH + 400; // fuera de la pantalla
            }
            else if (enemyBullet->x < 0) {
                enemyBullet->health = 0; // desactivar bala
                enemyBullet->x = SCREEN_WIDTH + 400; // fuera de la pantalla
                enemyBullet->y = 0;
            }
        }

        render(enemyBullet->texture, enemyBullet->x, enemyBullet->y);
    }
}


void playerLogic() {
    if (app.up && player->y > 0 && player->health == 1) {
        player->y -= PLAYER_SPEED;
    }
    if (app.down && player->y < SCREEN_HEIGHT - 45 && player->health == 1) {
        player->y += PLAYER_SPEED;
    }
    if (app.left && player->x > 0 && player->health == 1) {
        player->x -= PLAYER_SPEED;
    }
    if (app.right && player->x < SCREEN_WIDTH - 45 && player->health == 1) {
        player->x += PLAYER_SPEED;
    }

    // colision entre jugador y enemigo
    for (int i = 0; i < ENEMY_COUNT; i++) {
        if (checkEntityCollision(player, &enemyList[i]) && player->health == 1) {
            player->health = 0; // desactivar jugador
            player->x = SCREEN_WIDTH + 400; // fuera de la pantalla 
            player->y = 0;
            return;
        }
    }

    render(player->texture, player->x, player->y);
}


void playerBulletLogic() {
    for (int i = 0; i < 10; i++) { // recorrer la lista de balas
        bullet = &bulletList[i]; // obtener bala actual

        if (bullet->health == 1) {
            bullet->x += PLAYER_BULLET_SPEED; // mover bala
            if (bullet->x > SCREEN_WIDTH) {
                bullet->health = 0; // desactivar bala
            }

            // si la bala colisiona con un enemigo activo
            for (int j = 0; j < ENEMY_COUNT; j++) {
                if (enemyList[j].health == 1 &&  checkEntityCollision(bullet, &enemyList[j])) {
                    // borrar bala y enemigo
                    bullet->x = SCREEN_WIDTH + 400; // fuera de la pantalla
                    enemyList[j].x = SCREEN_WIDTH + 400; // fuera de la pantalla
                }
            }

        }
        else if (app.fire && player->fireCooldown > 9 && player->health == 1) { 
            bullet->x = player->x + 30;
            bullet->y = player->y + 18;
            bullet->health = 1; // activar bala

            player->fireCooldown = 0; // reiniciar velocidad de disparo
        }

        render(bullet->texture, bullet->x, bullet->y); // dibujar bala
    }

    player->fireCooldown++; // incrementar
}


void logic() {
    playerLogic();
    playerBulletLogic();
    enemyLogic();
    enemyBulletLogic();
}


void prepareScene(void) {
    SDL_SetRenderDrawColor(app.renderer, 40, 40, 60, 255);
    SDL_RenderClear(app.renderer);
}


void presentScene(void) {
    SDL_RenderPresent(app.renderer);
}


int initSDL() {
    if (SDL_Init(SDL_INIT_EVERYTHING)) {
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        return FALSE;
    }

    app.window = SDL_CreateWindow("Shooter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_BORDERLESS);
    if (!app.window) {
        printf("Failed to open %d x %d window: %s\n", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_GetError());
        return FALSE;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    app.renderer = SDL_CreateRenderer(app.window, -1, SDL_RENDERER_ACCELERATED);
    if (!app.renderer) {
        printf("Failed to create renderer: %s\n", SDL_GetError());
        return FALSE;
    }

    return TRUE;
}


int main(int argc, char* argv[]) {

    game_is_running = initSDL();

    initStage();

    while(game_is_running) {

        prepareScene(); // preparar la escena de juego
        doInput(); // manejar la entrada del usuario
        logic(); // logica del juego
        presentScene(); // presentar la escena de juego

        SDL_Delay(8); // esperar milisegundos
    }

    destroy_window();

    return 0;
}