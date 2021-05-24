#include "scene.hpp"
#include "ui.hpp"
#include "player.hpp"
#include <sstream>


SceneSystem::SceneSystem(ivec2 w_px, WorldSystem* w) {
    window_size_px = w_px;
    world = w;

    ///////////////////////////////////////
    // Initialize GLFW
    auto glfw_err_callback = [](int error, const char* desc) { std::cerr << "OpenGL:" << error << desc << std::endl; };
    glfwSetErrorCallback(glfw_err_callback);
    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

    //-------------------------------------------------------------------------
    // GLFW / OGL Initialization, needs to be set before glfwCreateWindow
    // Core Opengl 3.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, 0);



    // Create the main window (for rendering, keyboard, and mouse input)
    window = glfwCreateWindow(window_size_px.x, window_size_px.y, "Five Wonders", nullptr, nullptr);
    if (window == nullptr)
        throw std::runtime_error("Failed to glfwCreateWindow");
    world->window = window;
    world->showPause = [this](bool a){this->pause(a);};
    world->showSpellSelect = [this](ECS::Entity player_character, Spell::SpellEffect spell_effect){this->spellSelect(player_character, spell_effect);};
    world->endLevel = [this](){
        this->world->endBattle();
        WorldSystem::sound.stopMusic();
        queued_cutscenes.push_back(Cutscene("cutscenes/exit.png"));
        end_cutscenes = [this](){
            WorldSystem::sound.play("bgm_menu");
            worldmap();
            };
        cutscene();
        };
    // Setting callbacks to member functions (that's why the redirect is needed)
    // Input is handled using GLFW, for more info see
    // http://www.glfw.org/docs/latest/input_guide.html
    glfwSetWindowUserPointer(window, this);
    auto key_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2, int _3) { ((SceneSystem*)glfwGetWindowUserPointer(wnd))->on_key(_0, _1, _2, _3); };
    auto cursor_pos_redirect = [](GLFWwindow* wnd, double _0, double _1) { ((SceneSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_move({ _0, _1 }); };
    auto mouse_button_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2) { ((SceneSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_button(_0, _1, _2); };
    glfwSetKeyCallback(window, key_redirect);
    glfwSetCursorPosCallback(window, cursor_pos_redirect);
    glfwSetMouseButtonCallback(window, mouse_button_redirect);
}

void SceneSystem::step(float elapsed_ms, vec2 window_size_in_game_units) {
    // Updating window title with points
    std::stringstream title_ss;
    title_ss << "Five Wonders";
    glfwSetWindowTitle(window, title_ss.str().c_str());
    switch(scene) {
        case TITLE: // no every step logic, only observers for buttons.
            break;
        case LEVEL_SELECT:
            break;
        case CUTSCENE:
            break;
        case BATTLE:
            world->step(elapsed_ms, window_size_in_game_units);
            break;
        case EDITOR:
            break;
    }
}

void SceneSystem::title() {
    WorldSystem::sound.play("bgm_menu");
    Background::changeBackground("title_notext.png", vec2(2.96, 2.96));
    Button::createButton(vec2(230, 290), 120, 60, [this](){this->worldmap();}, "buttons/play.png");
    Graphic::createGraphic(vec2(380, 160), vec2(450, 150), "title.png");
}

void SceneSystem::battle(int area) {
    WorldSystem::in_game = true;
    world->setArea(area);
    WorldSystem::level_to_load = "area" + std::to_string(area) + ".json";
    world->restart();
    switch(area) {
        case 1: WorldSystem::sound.play("bgm_area1");
            break;
        case 2: WorldSystem::sound.play("bgm_area2");
            break;
        case 3: WorldSystem::sound.play("bgm_area3");
            break;
        case 4:
            WorldSystem::sound.play("bgm_shop");
            break;
        default: break;
    }
    scene = BATTLE;
}

void SceneSystem::battle(std::string level_to_load) {
    WorldSystem::in_game = true;
    WorldSystem::level_to_load = level_to_load;
    world->load_area(level_to_load);
    if (world_editor != nullptr) {
        delete world_editor;
        world_editor = nullptr;
    }
    switch(world->getArea()) {
        case 1: WorldSystem::sound.play("bgm_area1");
            break;
        case 2: WorldSystem::sound.play("bgm_area2");
            break;
        case 4:
            break;
        default: break;
    }

    world->restart();
    scene = BATTLE;
}

void SceneSystem::worldmap() {
    UISystem::showUI(UISystem::NONE);
    clear();
    scene = LEVEL_SELECT;
    Background::changeBackground("map/world_map_2.png", vec2(2.96, 2.96));
    Button::createButton(vec2(170, 160), 168, 120, [this](){this->area(0);}, "map/tutorial_tower.png"); //replace with tutorial area asset
    Button::createButton(vec2(460, 200), 168, 120, [this](){this->area(1);}, "map/ancient_library.png");
    Button::createButton(vec2(750, 240), 168, 120, [this](){this->area(2);}, "map/flooded_vault.png");
    Button::createButton(vec2(500, 440), 168, 120, [this](){this->area(3);}, "map/ethereal_keep.png");
    Button::createButton(vec2(800, 500), 168, 120, [this](){this->area(4);}, "map/the_shop.png");
    Button::createButton(vec2(1050, 50), 300, 30, [this](){editor();}, "map/editor.png");
}

void SceneSystem::area(int area) {
    clear();
    // Queue cutscenes:
    switch(area) {
        case 1:
            WorldSystem::sound.stopMusic();
//            world->setArea(1);
            WorldSystem::setArea(1);
            queued_cutscenes.push_back(Cutscene("cutscenes/area1/cutscene1-3.png"));
            queued_cutscenes.push_back(Cutscene("cutscenes/area1/cutscene1-2.png"));
            queued_cutscenes.push_back(Cutscene("cutscenes/area1/cutscene1-1.png"));
            end_cutscenes = [this](){battle(1);};
            break;
        case 2:
            WorldSystem::sound.stopMusic();
//            world->setArea(2);
            WorldSystem::setArea(2);
            queued_cutscenes.push_back(Cutscene("cutscenes/area2/cutscene2-2.png"));
            queued_cutscenes.push_back(Cutscene("cutscenes/area2/cutscene2-1.png"));
            end_cutscenes = [this](){battle(2);};
            break;
        case 0:
            WorldSystem::sound.stopMusic();
            WorldSystem::setArea(0);
            end_cutscenes = [this](){battle(0);};
            break;
        case 3:
            WorldSystem::sound.stopMusic();
        //            world->setArea(2);
            WorldSystem::setArea(3);
            end_cutscenes = [this](){battle(3);};
            break;
        case 4:
            WorldSystem::sound.stopMusic();
            WorldSystem::setArea(4);
            end_cutscenes = [this](){battle(4);};
        default:
            break;
    }
    cutscene();
}

void SceneSystem::pause(bool p) {
    if (p) {
        UISystem::showUI(UISystem::NONE);
        ECS::registry<PauseComponent>.emplace(UISystem::showGraphic("menu/overlay.png", window_size_px / 2, window_size_px));
        ECS::registry<PauseComponent>.emplace(UISystem::showGraphic("menu/game_paused.png", window_size_px / 2, {300, 100}));
        ECS::registry<PauseComponent>.emplace(Button::createButton(vec2(window_size_px.x/2, window_size_px.y/2 + 100), 240, 70,
                                                                   [this](){pause(false);}, "menu/resume.png"));
        ECS::registry<PauseComponent>.emplace(Button::createButton(vec2(window_size_px.x/2, window_size_px.y/2 + 200), 240, 70,
                                                                   [this](){world->endBattle();  WorldSystem::sound.play("bgm_menu"); worldmap(); },
                                                                   "menu/back_to_map.png"));
    } else {
        UISystem::showUI(UISystem::BATTLE);
        world->isGamePaused = false;
        for (auto& e : ECS::registry<PauseComponent>.entities) {
            ECS::ContainerInterface::remove_all_components_of(e);
        }
    }
}

void SceneSystem::spellSelect(ECS::Entity player_character, Spell::SpellEffect spell_effect) {
    // TODO: clean up spaghetti
    bool spell1_occupied = false;
    bool spell2_occupied = false;
    bool spell3_occupied = false;
    for (auto& entity : ECS::registry<BattleUI>.entities) {
        auto& b_ui = ECS::registry<BattleUI>.get(entity);
        if (b_ui.type == BattleUI::SPELL1) {
            spell1_occupied = true;
        } else if (b_ui.type == BattleUI::SPELL2) {
            spell2_occupied = true;
        } else if (b_ui.type == BattleUI::SPELL3) {
            spell3_occupied = true;
        }
    }

    auto& player = ECS::registry<Player>.get(player_character);
    UISystem::showUI(UISystem::NONE);
    ECS::registry<PauseComponent>.emplace(UISystem::showGraphic("menu/overlay.png", window_size_px / 2, window_size_px));
    ECS::registry<PauseComponent>.emplace(UISystem::showGraphic("menu/spell_select.png", vec2(window_size_px.x/2, window_size_px.y/4), {300, 100}));

    ECS::registry<PauseComponent>.emplace(Button::createButton(vec2(window_size_px.x/2, window_size_px.y/4 + 400), 160, 64,
                                                               [this](){pause(false);}, "spellselection/spellnone.png"));
    //    if (!spell1_occupied) {
        ECS::registry<PauseComponent>.emplace(Button::createButton(vec2(window_size_px.x/2, window_size_px.y/4 + 100), 160, 64,
                                                                   [&, spell_effect](){player.modify_spell_map(WorldSystem::SpellButton::ONE, spell_effect); world->updateSpellButton(WorldSystem::SpellButton::ONE, spell_effect); pause(false);},
                                                                   "spellselection/spell1.png"));
//    }
//    if (!spell2_occupied) {

    int size = ECS::registry<JODBUTTON>.size()-1;
    if(size < 4) return;
        ECS::registry<PauseComponent>.emplace(Button::createButton(vec2(window_size_px.x/2, window_size_px.y/4 + 200), 160, 64,
                                                                   [&, spell_effect](){player.modify_spell_map(WorldSystem::SpellButton::TWO, spell_effect); world->updateSpellButton(WorldSystem::SpellButton::TWO, spell_effect); pause(false);},
                                                                   "spellselection/spell2.png"));
//    }
//    if (!spell3_occupied) {

    if(size < 5) return;
        ECS::registry<PauseComponent>.emplace(Button::createButton(vec2(window_size_px.x/2, window_size_px.y/4 + 300), 160, 64,
                                                                   [&, spell_effect](){player.modify_spell_map(WorldSystem::SpellButton::THREE, spell_effect); world->updateSpellButton(WorldSystem::SpellButton::THREE, spell_effect); pause(false);},
                                                                   "spellselection/spell3.png"));

//    }
}

void SceneSystem::cutscene() {
    scene = CUTSCENE;
    clear();
    Button::createButton(vec2(window_size_px/2), window_size_px.x, window_size_px.y,
                         [this](){this->cutscene();}, "cutscenes/continue_button.png");
    if (!queued_cutscenes.empty()) {
        Cutscene c = queued_cutscenes.back();
        Background::changeBackground(c.img_path, vec2(2.5, 2.5));
        queued_cutscenes.pop_back();
    } else {
        end_cutscenes();
    }
}

void SceneSystem::editor() {
    clear();
    if (world_editor != nullptr) {
        delete world_editor;
        world_editor = nullptr;
    }
    Background::changeBackground("editor_background.png", vec2(2.96, 2.96));
    // Set scene to EDITOR once a json has been selected
    world_editor = new WorldEditSystem(world, window_size_px);
    world_editor->editLevel = [this](){
        clear();
        this->scene = EDITOR;
        Background::changeBackground("ui/editor/editor_ingame_background.png", vec2(20, 20));
        ECS::registry<Tooltip>.insert(Button::createButton({50,50}, 72, 72,
                             [this](){
            this->scene = LEVEL_SELECT;
            this->editor();
            },
                             "buttons/back.png"), Tooltip({50,50}, "Exit level (does not save!)"));
    };
    world_editor->playLevel = [this](std::string s){
        clear();
        this->battle(s);
    };
    world_editor->showLevels();
    //scene = EDITOR;
    Button::createButton({50,50}, 72, 72, [this](){free(world_editor); world_editor = nullptr; this->worldmap();}, "buttons/back.png");
}

void SceneSystem::clear() {
    UISystem::clearText();
    // Removes all buttons
    while (ECS::registry<Motion>.entities.size()>0)
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<Motion>.entities.back());
}

void SceneSystem::awake() {
    world->awake();

    UISystem::header  = Font::load("data/fonts/header/Marediv.ttf");
    UISystem::sans  = Font::load("data/fonts/sans/Connection.ttf");
    UISystem::serif = Font::load("data/fonts/serif/ConnectionSerif.ttf");
    title();
}

SceneSystem::~SceneSystem() {
    // Close the window
    glfwDestroyWindow(window);
}

void SceneSystem::on_key(int key, int _2, int action, int mod) {

    switch(scene) {
        case TITLE: // no every step logic, only observers for buttons.
            break;
        case LEVEL_SELECT:
            break;
        case CUTSCENE:
            break;
        case BATTLE:
            world->on_key(key,_2,action,mod);
            break;
        case EDITOR:
            world_editor->on_key(key,_2,action,mod);
    }
}

void SceneSystem::on_mouse_move(vec2 mouse_pos) {
    world->mouse_position = mouse_pos;
    switch(scene) {
        case TITLE: // no every step logic, only observers for buttons.
            break;
        case LEVEL_SELECT:
            break;
        case CUTSCENE:
            break;
        case BATTLE:
            world->on_mouse_move(mouse_pos, true, 600);
            break;
        case EDITOR:
            world_editor->on_mouse_move(mouse_pos);
            break;
    }
}

void SceneSystem::on_mouse_button(int button, int action, int mods) {
    if (checkButtons(button, action, mods)) return;
    switch(scene) {
        case TITLE: // no every step logic, only observers for buttons.
            break;
        case LEVEL_SELECT:
            break;
        case CUTSCENE:
            break;
        case BATTLE:
            world->on_mouse_button(button,action,mods);
            break;
        case EDITOR:
            world_editor->on_mouse_button(button,action,mods);
            break;
    }
}

bool SceneSystem::checkButtons(int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        if (world->mouseHandler.handle_click(true, world->mouse_position, [this]() { this->world->play_sound(); })) {
            return true;
        }
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        world->mouseHandler.handle_click(false, world->mouse_position, [this]() { this->world->play_sound(); });
    }
    return false;
}


Cutscene::Cutscene(std::string path) {
    img_path = path;
}
