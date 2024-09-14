#include "raylib.h"
#include "raymath.h"
#include <stdio.h>

#define STEP 5

#define SCREEN_WIDTH 800.0f
#define SCREEN_HEIGHT 500.0f

#define PLAYER_WIDTH 150.0f
#define PLAYER_HEIGHT 20.0f
#define PLAYER_ANGLE_CHANGE 0.08f
#define PLAYER_SPEED_CHANGE 1.05f
#define PLAYER_LIFES 3

#define PUCK_SIZE 15.0f
#define PUCK_COLOR RED

#define BRICK_COLOR BLACK
#define BRICKS_PER_ROW 8
#define BRICKS_PER_COLUMN 4
#define GAP 20.0f
#define BRICK_WIDTH ((SCREEN_WIDTH - (BRICKS_PER_ROW+1)*GAP)/BRICKS_PER_ROW)
#define BRICK_HEIGHT (BRICK_WIDTH / 3)
#define BRICK_ALPHA (atan2(BRICK_HEIGHT, BRICK_WIDTH))

#define INIT_SPEED 5
#define INIT_ANGLE_MIN (PI/2+0.2f)
#define INIT_ANGLE_MAX (PI/2+0.5f)

typedef struct {
	Vector2 position;
	Vector2 speed;
	bool active;
} Puck;

typedef struct {
	Vector2 position;
	int lifes;
} Player;

typedef struct {
	Vector2 position;
	bool active;
} Brick;

typedef enum {START, PLAYING, GAME_OVER, VICTORY} GameState;

typedef struct {
	Player player;
	Brick bricks[BRICKS_PER_ROW][BRICKS_PER_COLUMN];
	Puck puck;
	GameState state;
} Game;

Game game;

#define M 1000000.0f
float GetRandomFloat(float min, float max) {
	return (float)GetRandomValue((int)(M * min), (int)(M * max))/M;
}

void DrawLifeBar() {
	Vector2 dims = {PUCK_SIZE, PUCK_SIZE};
	for (int i=0; i<game.player.lifes; ++i) {
		Vector2 position = {PUCK_SIZE + 2 * i * PUCK_SIZE, SCREEN_HEIGHT - 2 * PUCK_SIZE};
		DrawRectangleV(position, dims, PUCK_COLOR);
	}
}

void SetBricksPosition() {
	for (int i=0; i<BRICKS_PER_ROW; ++i) {
		for (int j=0; j<BRICKS_PER_COLUMN; ++j) {
			game.bricks[i][j].position.x = GAP + i * (GAP + BRICK_WIDTH);
			game.bricks[i][j].position.y = GAP + j * (GAP + BRICK_HEIGHT);
			game.bricks[i][j].active = true;
		}
	}
}

typedef enum {COL_LEFT, COL_RIGHT, COL_TOP, COL_BOT, NONE} Collison;

Collison RectCollision(Rectangle rect1, Rectangle rect2) {
	if (CheckCollisionRecs(rect1, rect2)) {
		float dx = rect1.x + rect1.width/2 - (rect2.x + rect2.width/2);
		float dy = rect1.y + rect1.height/2 - (rect2.y + rect2.height/2);
		float fi = atan2(dy, dx);
		if (fi<0) {
			fi += 2 * PI;
		}
		float rec1_fi = atan2(rect1.height, rect1.width);
		if (rec1_fi<=fi && fi<PI-rec1_fi) {
			return COL_TOP;
		}
			
		if	(PI+rec1_fi<=fi && fi<2*PI-rec1_fi) {
			return COL_BOT;
		}

		if (fi<rec1_fi || fi>=2*PI-rec1_fi) {
			return COL_RIGHT;
		}

		return COL_LEFT;

	} else {
		return NONE;
	}
}

void SetPuckPosition() {
	game.puck.position = (Vector2) {game.player.position.x + PLAYER_WIDTH/2 - PUCK_SIZE/2, game.player.position.y - PUCK_SIZE};
}

typedef enum {LEFT, RIGHT} Direction;

bool PlayerCanMove(Direction direction) {
	switch (direction) {
		case LEFT:
			return game.player.position.x - STEP >= 0;
		case RIGHT:
			return game.player.position.x + PLAYER_WIDTH + STEP <= SCREEN_WIDTH;
	}
}

void MovePlayer() {
	if (IsKeyDown(KEY_LEFT) && PlayerCanMove(LEFT)) {
		game.player.position.x -= STEP;
		if (!game.puck.active) {
			SetPuckPosition();
		}
	}

	if (IsKeyDown(KEY_RIGHT) && PlayerCanMove(RIGHT)) {
		game.player.position.x += STEP;
		if (!game.puck.active) {
			SetPuckPosition();
		}
	}
}

void MovePuck() {
	if (game.puck.active) {
		game.puck.position.x += game.puck.speed.x;
		game.puck.position.y += game.puck.speed.y;
	}

	Rectangle puck = {game.puck.position.x, game.puck.position.y, PUCK_SIZE, PUCK_SIZE};
	for (int i=0; i<BRICKS_PER_ROW; ++i) {
		for (int j=0; j<BRICKS_PER_COLUMN; ++j) {
			if (!game.bricks[i][j].active) continue;
			Rectangle brick = {game.bricks[i][j].position.x, game.bricks[i][j].position.y, BRICK_WIDTH, BRICK_HEIGHT};
			switch (RectCollision(brick, puck)) {
				case COL_LEFT:
				case COL_RIGHT:
					game.puck.speed.x *= -1;
					game.bricks[i][j].active = false;
					goto brick_collisons_over;
				case COL_TOP:
				case COL_BOT:
					game.puck.speed.y *= -1;
					game.bricks[i][j].active = false;
					goto brick_collisons_over;
			}
		}
	}

brick_collisons_over:

	//top wall
	if (game.puck.position.y < 0) {
		game.puck.speed.y *= -1;
	}

	//left wall
	if (game.puck.position.x < 0) {
		game.puck.speed.x *= -1;
	}

	//right wall
	if (game.puck.position.x + PUCK_SIZE > SCREEN_WIDTH) {
		game.puck.speed.x *= -1;
	}

	////////PLAYER COLLISION//////////
	Rectangle player = {game.player.position.x, game.player.position.y, PLAYER_WIDTH, PLAYER_HEIGHT};
	switch (RectCollision(player, puck)) {
		case COL_TOP:
			game.puck.speed.y *= -1;
			game.puck.speed = Vector2Scale(game.puck.speed, PLAYER_SPEED_CHANGE);
			game.puck.speed = Vector2Rotate(game.puck.speed, GetRandomFloat(-PLAYER_ANGLE_CHANGE, PLAYER_ANGLE_CHANGE));
			break;
		case COL_BOT:
			game.puck.speed.y *= -1;
			break;
		case COL_LEFT:
		case COL_RIGHT:
			game.puck.speed.x *= -1;
			break;
	}

	if (game.puck.position.y>SCREEN_HEIGHT) {
		game.player.lifes -= 1;
		game.puck.active = false;
		SetPuckPosition();
		if (game.player.lifes==0) {
			game.state = GAME_OVER;
		}
	}
}

void SetPuckSpeed() {
	game.puck.speed = (Vector2) {INIT_SPEED, 0};
	game.puck.speed = Vector2Rotate(game.puck.speed, GetRandomFloat(INIT_ANGLE_MIN, INIT_ANGLE_MAX));
}

void InitializeGame() {
	game.player.position = (Vector2) {SCREEN_WIDTH/2 - PLAYER_WIDTH/2, SCREEN_HEIGHT - PLAYER_HEIGHT - 3 * PUCK_SIZE};
	game.player.lifes = PLAYER_LIFES;
	SetPuckPosition();
	game.puck.active = false;
	SetBricksPosition();
}

#define FONT_SIZE 30
#define OFFSET 40

void DrawTextCenter(char *message) {
	Vector2 textSize = MeasureTextEx(GetFontDefault(), message, FONT_SIZE, 1);
	DrawText(message, SCREEN_WIDTH/2 - textSize.x/2 - OFFSET, SCREEN_HEIGHT/2 - textSize.y/2, FONT_SIZE, BLACK);
}

void DrawScreen() {
		BeginDrawing();

		ClearBackground(DARKGRAY);

		switch (game.state) {
			case START:
				DrawTextCenter("PRESS [SPACE] TO START");
				break;
			case GAME_OVER:
				DrawTextCenter("GAME OVER! PRESS [SPACE] TO PLAY AGAIN");
				break;
			case VICTORY:
				DrawTextCenter("VICTORY! PRESS [SPACE] TO PLAY AGAIN");
				break;
		}

		DrawLifeBar();
		DrawRectangleV(game.player.position, (Vector2){PLAYER_WIDTH, PLAYER_HEIGHT}, BLACK);
		DrawRectangleV(game.puck.position, (Vector2){PUCK_SIZE, PUCK_SIZE}, PUCK_COLOR);

		for (int i=0; i<BRICKS_PER_ROW; ++i) {
			for (int j=0; j<BRICKS_PER_COLUMN; ++j) {
				if (!game.bricks[i][j].active) continue;
				DrawRectangleV(game.bricks[i][j].position, (Vector2){BRICK_WIDTH, BRICK_HEIGHT}, BRICK_COLOR);
			}
		}
		EndDrawing();
}

void SetVictory() {
	bool allInactive = true;
	for (int i=0; i<BRICKS_PER_ROW; ++i) {
		for (int j=0; j<BRICKS_PER_COLUMN; ++j) {
			if (game.bricks[i][j].active) {
				allInactive = false;
			}
		}
	}

	if (allInactive) {
		game.state = VICTORY;
	}
}

int main(void) {
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "X");
	SetTargetFPS(60);
	InitializeGame();
	game.state = START;

	while (!WindowShouldClose())	{
		if (game.state==PLAYING) {
			if (IsKeyDown(KEY_SPACE) && !game.puck.active) {
				game.puck.active = true;
				SetPuckSpeed();
			}

			MovePlayer();
			MovePuck();
			SetVictory();
		}

		if (game.state!=PLAYING && IsKeyDown(KEY_SPACE)) {
			InitializeGame();
			game.state = PLAYING;
		}

		DrawScreen();
	}

	CloseWindow();
	return 0;
}
