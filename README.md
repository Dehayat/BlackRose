# GameEngine

ECS Game Engine for studying games, game engines, and ECS.
This is still a work in progress and mainly used for educational purposes and not for practical or production use.

## Setup
To Run the the game you must configure the SDL library locations in the project settings

## Usage

Setting The inital entites should be done in the Game.cpp file in Setup function. Here you should add all the systems you want to run as well as create any entities that should be alive at the beggining of the game. Also here you should load your resources like textures.
to create an entity use the registry->CreateEntity function. This will return and entity which you can components to using the AddComponent<ComponentType> function of the entity. in the function parameters of the AddComponent function you should send the parameters of the constructor of ComponentType.
  
You must Remember to Update The systems where needed. Most systems should be updated in the Update function but for example the render system should be updated in the render function.
#   B l a c k R o s e  
 