#include <SDL2/SDL.h>
#include "Gameshell.h"
#include "Healthpack.h"
#include "Inventory.h"
#include "Armor.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <cmath>

GameShell::GameShell()
{
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("sdl-alg-sd", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    screen = SDL_GetWindowSurface(window);
    tileset = SDL_LoadBMP("tileset.bmp");
    lifebar = SDL_LoadBMP("lifebar.bmp");
    gameover = SDL_LoadBMP("gover.bmp");
    gamewon = SDL_LoadBMP("gwin.bmp");

    Uint32 color = SDL_MapRGB(lifebar->format, 0xFF, 0x00, 0xFF);
    SDL_SetColorKey(lifebar, SDL_TRUE, color);
    SDL_SetColorKey(gameover, SDL_TRUE, color);
    SDL_SetColorKey(tileset, SDL_TRUE, color);
    SDL_SetColorKey(gamewon, SDL_TRUE, color);
    SDL_Rect tile;
    tile.x = tile.y = 0;
    tile.w = tile.h = TILE_HEIGHT;

    for(int i = 0; i < 11; i++)
    {
        bkgtiles[i] = tile;
        tile.x += TILE_WIDTH;
    }

    // creating life bar types
    SDL_Rect bartile;
    bartile.x = 0;
    bartile.y = 0;
    bartile.h = 20;
    bartile.w = 100;

    for(int i = 0; i < 7; i++)
    {
        bartype.push_back(bartile);
        bartile.y += 20;
    }

    col = new CollisionDetector();
    itemlist.push_back(10); // 8 = medkit
    itemlist.push_back(8); // 10 = armor
}

GameShell::~GameShell()
{
    SDL_FreeSurface(lifebar);
    SDL_FreeSurface(gameover);
    SDL_FreeSurface(tileset);
    delete col;
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void GameShell::refresh()
{
    SDL_UpdateWindowSurface(window);
}

void GameShell::loadMap()
{
    SDL_Rect srctile, destile;
    std::ifstream mapfile("map.txt");
    int nr, id = 1;
    mapfile >> layerNumber;
    std::vector<Tile*> layer;

    for(unsigned int i = 0; i < layerNumber; i++)
    {
        destile.x = -TILE_WIDTH;
        destile.y = 0;
        layer.clear();
        for(int j = 0; j < MAXTILES; j++)
        {
            mapfile >> nr;
            if(nr == 1)
            {
                int posx = j % (SCREEN_WIDTH / TILE_WIDTH);
                int posy = j / (SCREEN_WIDTH / TILE_WIDTH);
                exit.x = posx * TILE_WIDTH;
                exit.y = posy * TILE_HEIGHT;
                std::cout<<"XEXIT = " <<exit.x <<" "<<"YEXIT = " << exit.y<<"\n";
            }

            srctile = bkgtiles[nr];
            destile.x += TILE_WIDTH;

            Tile* tile = new Tile(destile.x, destile.y);
            tile->id = id++;
            tile->type = nr;

            if(nr == TRANSPARENCY)
            {
                tile->setTransparency(true);
            }

            if(isItem(nr) == true)
            {
                tile->setItem(true);
            }

            layer.push_back(tile);

            SDL_BlitSurface(tileset, &srctile, screen, &destile);
            if(destile.x >= SCREEN_WIDTH - TILE_WIDTH)
            {
                destile.y += TILE_WIDTH;
                destile.x = -TILE_WIDTH;
            }
        }
        tiles.push_back(layer);
    }
    mapfile.close();
    SDL_UpdateWindowSurface(window);
}

void GameShell::repaintTile(SDL_Rect& coord)
{
    // tiles[i]; i = 20 * Y + X;
    //(tiles[i][(coord.x / TILE_WIDTH) + 20 * (coord.y / TILE_HEIGHT)])->type
    for(unsigned int i = 0; i < layerNumber; i++)
        SDL_BlitSurface(tileset, &bkgtiles[(tiles[i][(coord.x / TILE_WIDTH) + 20 * (coord.y / TILE_HEIGHT)])->type], screen, &coord);
}

void GameShell::action()
{
    loadMap();
    bob.draw(screen);
    zombie.draw(screen);
    updateLifebar(screen);
    refresh();
    bool playing = true; // is the player alive?
    while(true)
    {
        int start = SDL_GetTicks();
        actions.handleEvents();
        if(playing == false)
        {
            if(actions.exitGame() == true)
            {
                break;
            }
        }
        else
        {
            if(actions.exitGame() == true)
            {
                break;
            }

            else
            if(actions.direction.x != 0 || actions.direction.y != 0)
            {
                if(col->detect(bob.coord, actions.direction, tiles) == false)
                {
                    repaintTile(bob.coord);
                    bob.move(actions.direction, screen);
                    repaintTile(zombie.coord);
                    if(bob.coord.x == exit.x && bob.coord.y == exit.y)
                    {
                        gameWon();
                        playing = false;
                    }

                    else
                    {
                        if(near_exit(bob) == true || near_player(bob) == true) // The zombie will start the chase
                        {
                            int zombie_old_x = zombie.coord.x;
                            int zombie_old_y = zombie.coord.y;
                            Tile* t = findPath(tiles[0][(zombie.coord.x + zombie.coord.y * (SCREEN_WIDTH / TILE_WIDTH))/32],
                                       tiles[0][(bob.coord.x + bob.coord.y * (SCREEN_WIDTH / TILE_WIDTH))/32]);


                            if (t == NULL)
                                std::cout << "t e NULL :(\n";
                            else
                            {
                                // update zombie coordinates
                                zombie.coord.x = t->coord.x;
                                zombie.coord.y = t->coord.y;
                                std::cout<< "PREV COORDS: X = " <<zombie_old_x<<" | Y = "<<zombie_old_y<<"\n";
                                std::cout<< "NEW COORDS: X = " <<zombie.coord.x<<" | Y = "<<zombie.coord.y<<"\n";


                                // set the frame according to the direction
                                if(zombie.coord.x - zombie_old_x == TILE_WIDTH)
                                {
                                    zombie.last_frame = 1;
                                    std::cout<<"RIGHT\n";
                                }
                                else
                                if(zombie.coord.x - zombie_old_x == -TILE_WIDTH)
                                {
                                    zombie.last_frame = 3;
                                    std::cout<<"LEFT\n";
                                }
                                else
                                if(zombie.coord.y - zombie_old_y == -TILE_HEIGHT)
                                {
                                    zombie.last_frame = 0;
                                    std::cout<<"UP\n";
                                }
                                else
                                if(zombie.coord.y - zombie_old_y == TILE_HEIGHT)
                                {
                                    zombie.last_frame = 2;
                                    std::cout<<"DOWN\n";
                                }
                            }
                        }

                        else // The zombie will patrol
                        {
                            SDL_Rect temp = zombie.move(screen);
                            repaintTile(zombie.coord);

                            if (col->detect(zombie.coord, temp, tiles) == false)
                            {
                                zombie.coord.x += temp.x * TILE_WIDTH;
                                zombie.coord.y += temp.y * TILE_HEIGHT;
                            }
                        }

                        zombie.draw(screen);
                        bob.draw(screen);

                        // zombie attacks the player
                        if(bob.coord.x == zombie.coord.x && bob.coord.y == zombie.coord.y)
                        {
                            deductHealth(zombie.damage);
                            // repair the 3 tiles which are under the lifebar
                            SDL_Rect repair;
                            repair.x = (SCREEN_WIDTH - BAR_WIDTH) / 2 - 14;
                            repair.y = 0;
                            for(int i = 0; i < 4; i++)
                            {
                                repaintTile(repair);
                                repair.x += TILE_WIDTH;
                            }

                            // update the player's lifebar
                            updateLifebar(screen);
                        }
                    }
                }

                refresh();
            }

            else
            {
                if(actions.use_item == true)
                {
                    if (bob.inventory.cursor != NULL)
                    {
                        bob.inventory.cursor->val->use(bob);
                        bob.inventory.Delete();
                        if (bob.inventory.cursor != NULL)
                            bob.inventory.draw(bkgtiles, tileset, screen, bob.inventory.cursor->val->type);
                        else
                            bob.inventory.draw(bkgtiles, tileset, screen, 9);
                    }

                    updateLifebar(screen);
                }

                if(actions.left_inv_arrow == true && bob.inventory.cursor != NULL)
                {
                    bob.inventory.moveLeft(bkgtiles, tileset, screen);
                    bob.inventory.draw(bkgtiles, tileset, screen, bob.inventory.cursor->val->type);
                }

                if(actions.right_inv_arrow == true && bob.inventory.cursor != NULL)
                {
                    bob.inventory.moveRight(bkgtiles, tileset, screen);
                    bob.inventory.draw(bkgtiles, tileset, screen,  bob.inventory.cursor->val->type);
                }

                refresh();

                int layer_numb = layerNumber - 1;
                int tile_numb = bob.coord.y / 32 * 20 + (bob.coord.x / 32);
                if((tiles[layer_numb][tile_numb])->hasItem() == true)
                {
                    bob.pick(tiles[layer_numb][tile_numb]);
                    int type = (tiles[layer_numb][tile_numb])->type;
                    (tiles[layer_numb][tile_numb])->type = TRANSPARENCY;

                    repaintTile((tiles[layer_numb][tile_numb])->coord);
                    bob.draw(screen);

                    SDL_Rect dest;
                    dest.x = 14 * TILE_WIDTH;
                    dest.y = 0;
                    repaintTile(dest);

                    bob.inventory.draw(bkgtiles, tileset, screen, type);
                    refresh();
                }
            }


            actions.reset();
        }

        if(bob.health <= 0 && playing == true)
        {
            SDL_Rect bloodpos;
            bloodpos.x = bloodpos.y = 0;
            bloodpos.w = bloodpos.h = TILE_HEIGHT;
            repaintTile(zombie.coord);
            zombie.draw(screen);
            SDL_BlitSurface(tileset, &bkgtiles[5], screen, &zombie.coord);
            gameOver();
            playing = false;
            refresh();
        }

        if(1000/FPS > SDL_GetTicks() - start)
        {
            SDL_Delay(1000/FPS - (SDL_GetTicks() - start));
        }
    }
}

bool GameShell::deductHealth(unsigned int value)
{
    if(bob.health <= 0)
        return true;
    else
    {
        if (bob.invulnerable > 0)
            bob.invulnerable--;
        else
            bob.health -= value;

        if(bob.health <= 0)
            return true;
        else
            return false;
    }
}

void GameShell::updateLifebar(SDL_Surface* screen)
{
    SDL_Rect barpos;
    barpos.x = (SCREEN_WIDTH - BAR_WIDTH) / 2;
    barpos.y = 6; // 20 is the HEIGHT of the lifebar

    if(bob.health <= 100 && bob.health >= 80)
    {
        bartype[0].w = bob.health;
        SDL_BlitSurface(lifebar, &bartype[0], screen, &barpos);
    }

    else
    if(bob.health <= 79 && bob.health >= 60)
    {
        bartype[1].w = bob.health;
        SDL_BlitSurface(lifebar, &bartype[1], screen, &barpos);
    }

    else
    if(bob.health <= 59 && bob.health >= 40)
    {
        bartype[2].w = bob.health;
        SDL_BlitSurface(lifebar, &bartype[2], screen, &barpos);
    }

    else
    if(bob.health <= 39 && bob.health >= 20)
    {
        bartype[3].w = bob.health;
        SDL_BlitSurface(lifebar, &bartype[3], screen, &barpos);
    }

    else
    if(bob.health <= 19 && bob.health >= 1)
    {
        bartype[4].w = bob.health;
        SDL_BlitSurface(lifebar, &bartype[4], screen, &barpos);
    }

    else
    if(bob.health <= 0)
    {
        bartype[5].w = 100;
        SDL_BlitSurface(lifebar, &bartype[5], screen, &barpos);
    }

    SDL_BlitSurface(lifebar, &bartype[6], screen, &barpos);
}

void GameShell::gameOver()
{
    SDL_Rect fullscreen;
    SDL_Rect position;
    SDL_Rect goverpos;
    fullscreen.x = fullscreen.y = 0;
    fullscreen.h = SCREEN_HEIGHT;
    fullscreen.w = SCREEN_WIDTH;
    //Uint32 color = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
    //SDL_FillRect(screen, &fullscreen, color);
    Uint8 alpha = 1;
    SDL_SetSurfaceBlendMode(screen, SDL_BLENDMODE_BLEND);
    SDL_SetSurfaceAlphaMod(screen, alpha);
    position.x = 80;
    position.y = 212;
    goverpos.x = goverpos.y = 0;
    goverpos.h = 55;
    goverpos.w = 480;
    SDL_BlitSurface(gameover, &goverpos, screen, &position);
}

void GameShell::gameWon()
{
    SDL_Rect fullscreen;
    SDL_Rect position;
    SDL_Rect goverpos;
    fullscreen.x = fullscreen.y = 0;
    fullscreen.h = SCREEN_HEIGHT;
    fullscreen.w = SCREEN_WIDTH;
    //Uint32 color = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
    //SDL_FillRect(screen, &fullscreen, color);
    Uint8 alpha = 1;
    SDL_SetSurfaceBlendMode(screen, SDL_BLENDMODE_BLEND);
    SDL_SetSurfaceAlphaMod(screen, alpha);
    position.x = 80;
    position.y = 212;
    goverpos.x = goverpos.y = 0;
    goverpos.h = 55;
    goverpos.w = 480;
    SDL_BlitSurface(gamewon, &goverpos, screen, &position);
}

bool GameShell::isItem(int value)
{
    for(unsigned int i = 0; i < itemlist.size(); i++)
    {
        if(itemlist[i] == value)
        {
            return true;
        }
    }
    return false;
}

Tile* GameShell::findPath(Tile*& start, Tile*& goal) {
    mark();
    int DIM = (SCREEN_WIDTH / TILE_WIDTH);
    std::queue<Tile*> q;
    q.push(start);
    Tile* current = new Tile(0,0);
    while (!q.empty())
    {
        current = q.front();
        q.pop();
        current->marked = true;
        if (current == goal)
            break;
        //right
        int index = (current->id) / DIM;
        if ( index == current->coord.y / TILE_HEIGHT && index <= (int)tiles[0].size() && (tiles[0][current->id])->type != 2 && (tiles[0][current->id])->marked == false)
        {
            q.push(tiles[0][current->id]);
            (tiles[0][current->id])->previous = current;
        }
        //left
        index = (current->id - 2) / DIM;
        if ( index == current->coord.y / TILE_HEIGHT && index >= 1 && (tiles[0][current->id - 2])->type != 2 && (tiles[0][current->id - 2])->marked == false)
        {
            q.push(tiles[0][current->id - 2]);
            (tiles[0][current->id - 2])->previous = current;
        }
        //up
        index = current->id - 1 + DIM;
        if (index < (int)tiles[0].size() && (tiles[0][current->id - 1 + DIM])->type != 2 && (tiles[0][current->id - 1 + DIM])->marked == false)
        {
            q.push(tiles[0][current->id - 1 + DIM]);
            (tiles[0][current->id - 1 + DIM])->previous = current;
        }
        //down
        index = current->id - 1 - DIM;
        if (index >= 1 && (tiles[0][current->id - 1 - DIM])->type != 2 && (tiles[0][current->id - 1 - DIM])->marked == false)
        {
            q.push(tiles[0][current->id - 1 - DIM]);
            (tiles[0][current->id - 1 - DIM])->previous = current;
        }

    }

    //reverting the path
    Tile* temp = current;
    Tile* prev =  NULL;

    while (temp != start)
    {
        Tile* aux = temp->previous;
        temp->previous = prev;
        prev = temp;
        temp = aux;
    }

    return prev;

}

void GameShell::mark()
{
    for (int i = 0, n = tiles[0].size(); i < n; i++)
            tiles[0][i]->marked = false;
}

bool GameShell::near_exit(Player& player)
{
    int xfinish = exit.x / TILE_WIDTH;
    int yfinish = exit.y / TILE_HEIGHT;

    int xpos = player.coord.x / TILE_WIDTH;
    int ypos = player.coord.y / TILE_HEIGHT;

    std::cout << "Manhattan: " << std::abs(xfinish - xpos) + std::abs(yfinish - ypos) << "\n";

    if(std::abs(xfinish - xpos) + std::abs(yfinish - ypos) <= CHASE_TRIGGER)
        return true;
    return false;
}

bool GameShell::near_player(Player& player)
{
    int xenem = zombie.coord.x / TILE_WIDTH;
    int yenem = zombie.coord.y / TILE_HEIGHT;

    int xplayer = player.coord.x / TILE_WIDTH;
    int yplayer = player.coord.y / TILE_HEIGHT;

    std::cout << "Manhattan_zombie: " << std::abs(xenem - xplayer) + std::abs(yenem - yplayer) << "\n";
    if(std::abs(xenem - xplayer) + std::abs(yenem - yplayer) <= ENEMY_PROXIMITY)
        return true;
    return false;
}
