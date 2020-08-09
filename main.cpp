#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_image.h>
#include <set>
#include <vector>
#include <algorithm>
#include <time.h>
#include <cmath>
#include <limits>
#include <string>

const int FRAME_RATE=30;

class SDL {
virtual bool proccess_event(SDL_Event e){
    if (e.type == SDL_QUIT){
        return false;
    } else {
        return true;
    }
}
virtual void start(){}
virtual void update(){}
virtual void draw(){}

public:
SDL_Window* window = NULL;
SDL_Renderer * renderer=NULL;
std::vector<SDL_Texture*> textures;
int width;
int height;
std::set<SDL_Keycode> keys;
SDL(int width, int height) : width(width), height(height) {}
bool init()
{
    //Initialization flag
    bool success = true;

    //Initialize SDL
    if( SDL_Init( SDL_INIT_VIDEO ) < 0)
    {
        printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
        success = false;
    }
    else
    {
        //Create window
        if ( SDL_CreateWindowAndRenderer( width, height, SDL_WINDOW_SHOWN, &window, &renderer ) != 0 )
        {
            printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
            success = false;
        } else {
            //Initialize PNG loading
            int imgFlags = IMG_INIT_PNG;
            if( !( IMG_Init( imgFlags ) & imgFlags ) )
            {
                printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
                success = false;
            }
        }
    }

    return success;
}

/*
Moves the execution into event loop that will call proccess_event, update and draw functions.
It will continue until proccess_event returns false.
*/
void start_event_loop(){
    start();
    int timer=0;
    SDL_Event e;
    while (true){
        while (SDL_PollEvent(&e)){
            if(!proccess_event(e)) {
                return;
            }
        }
        if(SDL_GetTicks()-timer>1000/FRAME_RATE){
            update();
            draw();
            timer=SDL_GetTicks();
        }
    }
}

/* 
Loads a texture into textures vector and returns it.
After destroying the SDL object the texture will be freed.
*/
SDL_Texture* load_texture(std::string path){
    //The final texture
    SDL_Texture* newTexture = NULL;

    //Load image at specified path
    SDL_Surface* loadedSurface = IMG_Load( path.c_str() );
    if(loadedSurface == NULL)
    {
        printf( "Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError() );
    }
    else
    {
        //Create texture from surface pixels
        newTexture = SDL_CreateTextureFromSurface( renderer, loadedSurface );
        if(newTexture == NULL)
        {
            printf( "Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError() );
        }

        //Get rid of old loaded surface
        SDL_FreeSurface( loadedSurface );
    }
    SDL_SetTextureBlendMode(newTexture, SDL_BLENDMODE_BLEND);
    textures.push_back(newTexture);
    return newTexture;
}

virtual ~SDL()
{
    for(int i=0; i<textures.size(); i++){
        SDL_DestroyTexture(textures[i]);
    }
    //Destroy window
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);

    //Quit SDL subsystems
    SDL_Quit();
}
};

class Point {
    public:
    int x, y;
    static float distance(Point a, Point b){
        int dx=b.x-a.x;
        int dy=b.y-a.y;
        return sqrtf(dx*dx+dy*dy);
    }
    static int distance_squared(Point a, Point b){
        int dx=b.x-a.x;
        int dy=b.y-a.y;
        return dx*dx+dy*dy;
    }
};

typedef Point Actor;

template <class T>
T in_range(T value, T left, T right){
    return value>=left && value<=right;
}

class Rect {
    public:
    int x, y, w, h;

    Rect()=default;
    Rect(int x, int y, int w, int h){
        this->x=x;
        this->y=y;
        this->w=w;
        this->h=h;
    }
    Rect(Point min, Point max){
        x=min.x;
        y=min.y;
        w=max.x-min.x;
        h=max.y-min.y;
    }
    static bool intersecting(Rect a, Rect b){
        return (in_range(a.x, b.x, b.x+b.w) || in_range(b.x, a.x, a.x+a.w))
            && (in_range(a.y, b.y, b.y+b.h) || in_range(b.y, a.y, a.y+a.h));
    }
    Point center(){
        return {x+w/2, y+h/2};
    }
};

int sign(int value){
    return value>=0 ? 1 : -1;
}

class Map {
bool **map;

void map_erase(int x, int y){
    if(x>=0 && x<width && y>=0 && y<height){
        map[y][x]=false;
    }
}

void carve_room(Rect room){
    for(int y=room.y; y<room.y+room.h; y++){
        for(int x=room.x; x<room.x+room.w; x++){
            map_erase(x, y);
        }
    }
}

int random_int(int min, int max){
    return min+rand()%max;
}

Rect random_room(int min_size, int max_size){
    Point position={
        random_int(1, width-min_size-1),
        random_int(1, height-min_size-1)
    };
    Point expansion_space={
        std::min(max_size-min_size, width-position.x-2),
        std::min(max_size-min_size, height-position.y-2)
    };
    return {position,
           {position.x+random_int(min_size, expansion_space.x),
            position.y+random_int(min_size, expansion_space.y)}
    };
}

void connect_rooms(Rect room_a, Rect room_b){
    Point a_center=room_a.center();
    Point b_center=room_b.center();
    int direction_x=sign(b_center.x-a_center.x);
    int direction_y=sign(b_center.y-a_center.y);
    for(int x=a_center.x; x!=b_center.x; x+=direction_x){
        map_erase(x, a_center.y);
    }
    for(int y=a_center.y; y!=b_center.y; y+=direction_y){
        map_erase(b_center.x, y);
    }
}

void drawCell(int x, int y){
    SDL_SetRenderDrawColor( sdl->renderer, 100, 100, 100, 100 );
    SDL_Rect cell;
    cell.x=x*tile_size;
    cell.y=y*tile_size;
    cell.w=tile_size;
    cell.h=tile_size;
    SDL_SetTextureColorMod(sdl->textures[0], 150, 150, 150);
    SDL_RenderCopy(sdl->renderer, sdl->textures[0], nullptr, &cell);
    //SDL_RenderFillRect( sdl->renderer, &cell);
}

public:
SDL* sdl;
int width;
int height;
int tile_size;
Map(int width, int height, int tile_size):
width(width),
height(height),
tile_size(tile_size)
{
    map=new bool*[height];
    for(int y=0; y<height; y++){
        map[y]=new bool[width];
    }
}
~Map(){
    for(int y=0; y<height; y++){
        delete[] map[y];
    }
    delete[] map;
}
bool get(int x, int y){
    if(x>=0 && x<width && y>=0 && y<height){
        return map[y][x];
    } else {
        return true;
    }
}
// returns entry point
Point generate_map(){
    // fill in the map
    for(int y=0; y<height; y++){
        for(int x=0; x<width; x++){
            map[y][x]=true;
        }
    }
    const int rooms_count=7;
    Rect rooms[rooms_count];
    for(int i=0; i<rooms_count; i++){
        Rect new_room;
        retry:
        {
            new_room=random_room(2, 5);
            for(int j=0; j<i; j++){
                if(Rect::intersecting(new_room, rooms[j])){
                    // try placing the room somewhere else
                    goto retry;
                }
            }
        }
        carve_room(new_room);
        connect_rooms(rooms[i-1], new_room);
        rooms[i]=new_room;
    }
    return rooms[rooms_count-1].center();
}
void draw()
{   
    for(int y=0; y<height; y++){
        for(int x=0; x<width; x++){
            if(map[y][x]){
                drawCell(x, y);
            }
        }
    }
}
};

float wrap(float value, float min, float max){
    float interval=max-min;
    return min+value-interval*floor((value-min)/interval);
}

float wrap_radians(float value){
    return wrap(value, 0.0f, 2*M_PI);
}

// returns the quarter in which the angle lies
// quarters are in following order:
// 3 | 4
// --+--
// 2 | 1
// because SDL inverts Y axis
int quarter(float angle){
    return ceilf(angle/(M_PI/2));
}

template<class T>
T reverse_lerp(T value, T min, T max){
    return (value-min)/(max-min);
}

struct RayBoxHitResult {
    Point hit_point;
    float point_on_wall;
    RayBoxHitResult(int x, int y, float point_on_wall): 
    hit_point({x, y}), point_on_wall(point_on_wall) {}
};

RayBoxHitResult ray_box_hit(Point ray_origin, float ray_direction, Point box_min, Point box_max){
    float a=tan(ray_direction);
    float b=ray_origin.y-a*ray_origin.x;
    int q=quarter(ray_direction);

    if(ray_origin.x<=box_min.x && (q==1 || q==4)) {
        int x=box_min.x;
        int y=a*x+b;
        if(y>=box_min.y&&y<=box_max.y){
            return {x, y, reverse_lerp<float>(y, box_min.y, box_max.y)};
        }
    }
    if(ray_origin.x>=box_max.x && (q==3 || q==2)) {
        int x=box_max.x;
        int y=a*x+b;
        if(y>=box_min.y&&y<=box_max.y){
            return {x, y, reverse_lerp<float>(y, box_min.y, box_max.y)};
        }
    }
    if(ray_origin.y<=box_min.y && (q==1 || q==2)){
        int y=box_min.y;
        int x=(y-b)/a;
        if(x>=box_min.x&&x<=box_max.x){
            return {x, y, reverse_lerp<float>(x, box_min.x, box_max.x)};
        }
    }
    if(ray_origin.y>=box_max.y && (q==3 || q==4)) {
        int y=box_max.y;
        int x=(y-b)/a;
        if(x>=box_min.x&&x<=box_max.x){
            return {x, y, reverse_lerp<float>(x, box_min.x, box_max.x)};
        }
    }
    return {0, 0, 0};
}

class Player {
struct Ray {
    Point hit_point;
    float distance, point_on_wall, distance_from_previous;
};
struct Vision {
    float field_of_view=1.5;
    int rays_count;
    int perspective_constant=10;
    Ray* rays;// length of each vision ray
} vision;
public:
SDL_Renderer * renderer;
SDL* sdl;
Map* map;
std::vector<Actor>* actors;
Point position; 
float rotation;
float speed=3;
float rotation_speed=0.1;
Player(SDL* sdl, Map* map, Point position, int rays_count){
    this->sdl=sdl;
    this->renderer=sdl->renderer;
    this->map=map;
    this->position=position;
    this->vision.rays_count=rays_count;
    this->vision.rays=new Ray[rays_count];
    this->rotation=0;
}
void draw_rays(){
    SDL_SetRenderDrawColor( renderer, 255, 255, 80, 255 );
    float step=this->vision.field_of_view/this->vision.rays_count;
    for(int i=0; i<this->vision.rays_count; i++){
        if(this->vision.rays[i].distance!=INFINITY) {
            float rotation=wrap_radians(this->rotation-this->vision.field_of_view/2+step*i);
            float length=this->vision.rays[i].distance;
            SDL_RenderDrawLine(renderer, this->position.x, this->position.y, 
            (int)((float)this->position.x+cos(rotation)*length), (int)((float)this->position.y+sin(rotation)*length));
        }
    }
}
// radians wrap around a circle so they can't be compared normally
bool in_range_radians(float value, float left, float right){
    if(right>left){
        return value>=left && value<=right;
    } else {
        return value<=left && value>=right;
    }
}
float x_on_screen(Point p){
    float player_point_angle=atan2(p.y-position.y, p.x-position.x);
    float leftmost_ray_point_angle=wrap_radians(player_point_angle-rotation);
    if(leftmost_ray_point_angle>M_PI){
        leftmost_ray_point_angle=-(2*M_PI-leftmost_ray_point_angle);
    }
    return (leftmost_ray_point_angle+vision.field_of_view/2)/vision.field_of_view;
}
bool in_screen_bounds(Point p){
    float x=x_on_screen(p);
    return x>=0 && x<=1;
}
bool in_sight(Point p){
    const float approximation=0.3f;

    int closest_ray_index=x_on_screen(p)*((float)vision.rays_count-1.0f);
    //closest_ray_index=std::max(closest_ray_index, 0);
    //closest_ray_index=std::min(closest_ray_index, vision.rays_count-1);
    if(closest_ray_index>=0 && closest_ray_index<vision.rays_count){
        float closest_ray_length=vision.rays[closest_ray_index].distance;
        return (closest_ray_length-Point::distance(position, p))>=-approximation;
    } else {
        return true;
    }
}
void draw_actors(){
    // sort actors by distance from player to make sure that the futher ones are drawn first
    std::sort(actors->begin(), actors->end(),[this](Actor a, Actor b){
        return Point::distance_squared(position, a)>Point::distance_squared(position, b);
    });
    for(int i=0; i<actors->size(); i++){
        float player_actor_angle=atan2(actors->at(i).y-position.y, actors->at(i).x-position.x);
        float perpendicular=wrap_radians(rotation+M_PI/2);

        Point leftmost_point={
            (int)(actors->at(i).x+cos(perpendicular)*map->tile_size/2),
            (int)(actors->at(i).y+sin(perpendicular)*map->tile_size/2)
        };
        Point rightmost_point={
            (int)(actors->at(i).x-cos(perpendicular)*map->tile_size/2),
            (int)(actors->at(i).y-sin(perpendicular)*map->tile_size/2)
        };

        if(in_screen_bounds(leftmost_point) || in_screen_bounds(rightmost_point)){
            float distance=Point::distance(position, actors->at(i));
            int size_on_screen=vision.perspective_constant*(float)sdl->height/distance;

            if(in_sight(actors->at(i)) && in_sight(leftmost_point) && in_sight(rightmost_point)){
                int brightness=std::min<int>(1000/distance, 155);
                SDL_Rect destination_rect={
                    (int)(sdl->width/2+x_on_screen(actors->at(i))*(sdl->width/2))-size_on_screen/2,
                    sdl->height/2-size_on_screen/2,
                    size_on_screen,
                    size_on_screen
                };
                SDL_SetTextureColorMod(sdl->textures[1], brightness, brightness, brightness);
                SDL_RenderCopy( renderer, sdl->textures[1], nullptr,  &destination_rect);
            }
            SDL_SetRenderDrawColor( renderer, 255, 255, 80, 255 );
            //SDL_RenderDrawLine(sdl->renderer, sdl->width/2+x_on_screen(rightmost_point)*(sdl->width/2), sdl->height/2, sdl->width/2+x_on_screen(leftmost_point)*(sdl->width/2), sdl->height/2);
        }
    }
}
void draw_first_person_view(){
    const int max_wall_height=sdl->height*vision.perspective_constant;
    const float picture_plane_distance=1.0f;
    float step=this->vision.field_of_view/this->vision.rays_count;
    int slice_width=sdl->width/2/this->vision.rays_count;
    int texture_w, texture_h;
    // SDL_QueryTexture only accesses struct fields so it can be called every frame
    SDL_QueryTexture(sdl->textures[0], NULL, NULL, &texture_w, &texture_h);
    for(int i=0; i<this->vision.rays_count; i++){
        if(this->vision.rays[i].distance!=INFINITY) {
            float rotation=wrap(this->vision.field_of_view/2+step*i, 0.0f, 2*(float)M_PI);
            float distance_from_plane=this->vision.rays[i].distance*sin(rotation);
            int slice_height=max_wall_height/distance_from_plane;
            int brightness=std::min<int>(1000/this->vision.rays[i].distance, 155);
            SDL_Rect slice_on_screen={
                sdl->width/2+slice_width*i,
                sdl->height/2-slice_height/2,
                slice_width,
                slice_height
            };
            SDL_SetTextureColorMod(sdl->textures[0], brightness, brightness, brightness);
            int texture_slice_width=(slice_width*slice_height)/texture_h;
            SDL_Rect source_rect={(int)((texture_w-texture_slice_width)*vision.rays[i].point_on_wall), 0, texture_slice_width, texture_h};
            SDL_RenderCopy( renderer, sdl->textures[0], &source_rect, &slice_on_screen );
        }
    }
    draw_actors();
}
void draw(){
    SDL_SetRenderDrawColor( renderer, 255, 255, 80, 255 );
    SDL_Rect cell;
    cell.x=this->position.x-map->tile_size/4;
    cell.y=this->position.y-map->tile_size/4;
    cell.w=map->tile_size/2;
    cell.h=map->tile_size/2;
    SDL_RenderFillRect(renderer, &cell);
    draw_rays();
}
void update_rays(){
    float step=this->vision.field_of_view/this->vision.rays_count;
    for(int i=0; i<this->vision.rays_count; i++){
        float rotation=wrap_radians(this->rotation-this->vision.field_of_view/2+step*i);
        int q=quarter(rotation);
        
        this->vision.rays[i]={{0, 0}, (float)INFINITY, 0.0f};
        for(int y=0; y<map->height; y++){
            for(int x=0; x<map->width; x++){
                int tile_x=x*map->tile_size+map->tile_size/2;
                int tile_y=y*map->tile_size+map->tile_size/2;
                if(map->get(x, y)/* && (
                    (q==1 && tile_x>=this->position.x && tile_y>=this->position.y)
                ||  (q==2 && tile_x<=this->position.x && tile_y>=this->position.y)
                ||  (q==3 && tile_x<=this->position.x && tile_y<=this->position.y)
                ||  (q==4 && tile_x>=this->position.x && tile_y<=this->position.y)
                )*/) {
                    RayBoxHitResult hit=ray_box_hit(this->position, rotation, {x*map->tile_size, y*map->tile_size}, {(x+1)*map->tile_size, (y+1)*map->tile_size});
                    int distance_squared=Point::distance_squared(position, hit.hit_point);
                    if(distance_squared<this->vision.rays[i].distance){
                        this->vision.rays[i]={
                            hit.hit_point,
                            (float)distance_squared,
                            hit.point_on_wall,
                            0.0f
                        };
                    }
                }
            }
        }
        this->vision.rays[i].distance=sqrtf(this->vision.rays[i].distance);
        if(i>0){
            this->vision.rays[i].distance_from_previous=Point::distance(this->vision.rays[i-1].hit_point, this->vision.rays[i].hit_point);
        }
    }
}
bool move_allowed(Point new_position){
    Rect collision_rect={
        new_position.x-map->tile_size/4, 
        new_position.y-map->tile_size/4,
        map->tile_size/2, 
        map->tile_size/2
    };
    #define TILE_COLLISION_(x, y) \
        (map->get(x, y) && Rect::intersecting(collision_rect, { (x)*map->tile_size, (y)*map->tile_size, map->tile_size, map->tile_size} ))
    #define TILE_COLLISION(ox, oy) \
        TILE_COLLISION_(new_position.x/map->tile_size+(ox), new_position.y/map->tile_size+(oy))
    return !(
        map->get(new_position.x/map->tile_size, new_position.y/map->tile_size)
    ||  TILE_COLLISION(-1, 1)
    ||  TILE_COLLISION(0, 1)
    ||  TILE_COLLISION(1, 1)
    ||  TILE_COLLISION(-1, 0)
    ||  TILE_COLLISION(1, 0)
    ||  TILE_COLLISION(-1, -1)
    ||  TILE_COLLISION(0, -1)
    ||  TILE_COLLISION(1, -1)
    );
}
void update(){
    #define KEY(keycode) if(sdl->keys.count(keycode))
    KEY(SDLK_LEFT){
        this->rotation-=this->rotation_speed;
    }
    KEY(SDLK_RIGHT){
        this->rotation+=this->rotation_speed;
    }
    this->rotation=wrap_radians(this->rotation);
    Point new_position=this->position;
    KEY(SDLK_UP){
        new_position.x+=cos(this->rotation)*this->speed;
        new_position.y+=sin(this->rotation)*this->speed;
    }
    KEY(SDLK_DOWN){
        new_position.x-=cos(this->rotation)*this->speed;
        new_position.y-=sin(this->rotation)*this->speed;
    }
    #undef KEY
    new_position.x=wrap(new_position.x, 0, sdl->width/2);
    new_position.y=wrap(new_position.y, 0, sdl->height);
    if(move_allowed(new_position)){
        position=new_position;
    } else if(move_allowed({position.x, new_position.y})){
        position={position.x, new_position.y};
    } else if(move_allowed({new_position.x, position.y})){
        position={new_position.x, position.y};
    }
    update_rays();
}
~Player(){
    delete[] this->vision.rays;
}
};

class Wolfenstein3D : public SDL {
Map* map;
Player player;
std::vector<Actor> actors;
void start(){
    Point entry_point=map->generate_map();
    place_actors();
    player.position={entry_point.x*map->tile_size, entry_point.y*map->tile_size};
    player.renderer=renderer;
    player.actors=&actors;
    load_texture("wall2.png");
    load_texture("wooden-box.png");
}
void place_actors(){
    const int actors_count=7;
    for(int i=0; i<actors_count; i++){
        Actor new_actor;
        retry:
        {
            new_actor={rand()%(width/2),rand()%height};
            int x_on_map=(float)new_actor.x/map->tile_size;
            int y_on_map=(float)new_actor.y/map->tile_size;
            if(map->get(x_on_map, y_on_map)){
                // try placing the actor somewhere else
                goto retry;
            }
        }
        actors.push_back(new_actor);
    }
}
void draw_actors(){
    for(int i=0; i<actors.size(); i++){
        SDL_Rect destination_rect={
            actors[i].x-map->tile_size/2,
            actors[i].y-map->tile_size/2,
            map->tile_size,
            map->tile_size
        };
        SDL_SetTextureColorMod(textures[1], 255, 255, 255);
        SDL_RenderCopy(renderer, textures[1], nullptr, &destination_rect);
    }
}
void draw()
{
    // Clear the window with a black background
    SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255 );
    SDL_RenderClear( renderer );

    // Show the window
    SDL_RenderPresent( renderer );

    /*
    SDL_SetRenderDrawColor( renderer, 50, 50, 50, 255 );
    SDL_Rect ceiling={0,0, width, height/2};
    SDL_RenderFillRect(renderer, &ceiling);

    SDL_SetRenderDrawColor( renderer, 100, 100, 100, 255 );
    SDL_Rect floor={0,height/2, width, height/2};
    SDL_RenderFillRect(renderer, &floor);
    */
    
    
    player.draw_first_person_view();

    // fill with black the left side of the screen just in case some part of the actor sprite in first person view went out of the right half
    SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255 );
    SDL_Rect left_side={0,0, width/2, height};
    SDL_RenderFillRect(renderer, &left_side);

    map->draw();
    player.draw();
    draw_actors();
    
    SDL_RenderPresent( renderer );
}
bool proccess_event(SDL_Event e){
    if (e.type == SDL_QUIT){
        return false;
    }
    if (e.type == SDL_KEYDOWN){
        keys.insert(e.key.keysym.sym);
    } else if(e.type == SDL_KEYUP){
        keys.erase(e.key.keysym.sym);
        if(e.key.keysym.sym==SDLK_ESCAPE){
            return false;
        }
    }
    return true;
}

void update(){
    player.update();
}
public:
Wolfenstein3D(int width, int height, Map* map): player(this, map, {0,0}, 100), SDL(width, height), map(map) {}
};

int main( int argc, char* args[] )
{
    Map map={20, 20, 21};
    Wolfenstein3D w{840, 420, &map };
    map.sdl=&w;
    //Start up SDL and create window
    if( !w.init() )
    {
        printf( "Failed to initialize!\n" );
    }
    else
    {
        srand(time(NULL));
        w.start_event_loop();
    }
    return 0;
}
