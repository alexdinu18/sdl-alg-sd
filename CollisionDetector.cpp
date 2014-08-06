#include "CollisionDetector.h"
#include "GameShell.h"

CollisionDetector::CollisionDetector()
{

}

CollisionDetector::~CollisionDetector()
{

}

bool CollisionDetector::detect(SDL_Rect& playerPos, SDL_Rect& direction, std::vector<int>& tiles)
{
	int index = (playerPos.x + TILE_WIDTH * direction.x) / TILE_WIDTH + (playerPos.y + TILE_HEIGHT * direction.y) / TILE_HEIGHT * 20;
	int index_x = (playerPos.x + TILE_WIDTH * direction.x) / TILE_WIDTH;
	int index_y = (playerPos.y + TILE_HEIGHT * direction.y) / TILE_HEIGHT;

    if(tiles[index] == 2)
	{
		return true;
	}
    else if (index_x < 0 || index_x > SCREEN_WIDTH / TILE_WIDTH - 1)
    {
        return true;
    }
    else if (index_y < 0 || index_y > SCREEN_HEIGHT / TILE_HEIGHT - 1)
    {
        return true;
    }
	else
	{
		return false;
	}
}

//Comment
