// internal
#include "player.hpp"
#include "boomerang.hpp"
#include "ui.hpp"
#include "spell.hpp"


Player::Player(ECS::Entity e, WorldSystem *w) {
    entity = e;
    world = w;

    Spell* PUSH = new Spell(Spell::SpellEffect::PUSH, 1, 2, 1, 1, true, true, false);
    Spell* SWAP = new Spell(Spell::SpellEffect::SWITCH, 1, 0, 1, 2, false, true, false);
    Spell* HEAL = new Spell(Spell::SpellEffect::HEAL, 10, 1, 0, 1, false, false, false);
    Spell* GRAPPLE = new Spell(Spell::SpellEffect::GRAPPLE_TO, 2, 2, 1, 3, true, false, false);
    
    spell_map = {
        {WorldSystem::SpellButton::ONE, GRAPPLE},
        {WorldSystem::SpellButton::TWO, SWAP},
        // switch to HEAL
        // {WorldSystem::SpellButton::THREE, HEAL},
        {WorldSystem::SpellButton::THREE, PUSH}
    };
}

void Player::modify_spell_map(WorldSystem::SpellButton selected, Spell::SpellEffect spell_effect) {
    Spell* new_spell;
    std::cout << spell_effect << std::endl;
    std::cout << Spell::SpellEffect::SWITCH << std::endl;
    if (spell_effect == Spell::SpellEffect::SWITCH) {
        new_spell = new Spell(Spell::SpellEffect::SWITCH, 1, 0, 1, 2, false, true, false);
    } else if (spell_effect == Spell::SpellEffect::GRAPPLE_TO) {
        new_spell = new Spell(Spell::SpellEffect::GRAPPLE_TO, 2, 2, 1, 3, true, false, false);
    } else if (spell_effect == Spell::SpellEffect::PUSH) {
        new_spell = new Spell(Spell::SpellEffect::PUSH, 1, 2, 1, 1, true, true, false);
    } else {
        new_spell = new Spell(Spell::SpellEffect::HEAL, 10, 1, 0, 1, false, false, false);
    }
    // TODO: more spells later
    spell_map[selected] = { new_spell };
    std::cout << "modified spell map" << std::endl;
}

void Player::show_tiles() {
    UISystem::disableHighlights();
    auto& turn = ECS::registry<Turn>.get(entity);
    TilePosition& tile_position = ECS::registry<TilePosition>.get(entity);
    vec2 character_tile = tile_position.grid_pos;
    std::vector<vec2> positions;
    switch (world->selected_button) {
        case 0: //MOVE
            show_move_tiles(character_tile, turn.movement);
            break;
        case 1: //MeleeATTACK
            if (turn.attacks <= 0) return;
            positions.push_back(tile_position.grid_pos + vec2(1, 0));
            positions.push_back(tile_position.grid_pos + vec2(-1, 0));
            positions.push_back(tile_position.grid_pos + vec2(0, 1));
            positions.push_back(tile_position.grid_pos + vec2(0, -1));
            UISystem::enableHighlights(positions, 2); 
            break;
        case 2: //RangeATTACK
            if (turn.attacks <= 0) return;
            show_ranged_tiles(character_tile, -1);
            break;
        case 3: //SPELL
        case 4:
        case 5:
            if (turn.spells >= spell_map[selected_spell]->cost)
                show_spell_tiles(character_tile);
        default:
            break;
    }
}

void Player::show_spell_tiles(vec2 playerPos) {
    std::vector<vec2> targets;
    if (spell_map[selected_spell]->requires_los) {
        for (auto pos : spell_map[selected_spell]->targetable_cells) {
            if (world->hasLoS(playerPos, pos + playerPos) && !WorldSystem::is_hole(pos+playerPos))
                targets.push_back(pos + playerPos);
        }
    } else {
        for (auto pos : spell_map[selected_spell]->targetable_cells) {
            if(!WorldSystem::is_hole(pos+playerPos))
                targets.push_back(pos + playerPos);
        }
    }
    if (spell_map[selected_spell]->requires_target) {
        std::vector<vec2> targetable;
        std::vector<vec2> non_targetable;
        for (auto target : targets) {
            try {
                WorldSystem::findCharacterAt(target);
                targetable.push_back(target);
            }
            catch (std::runtime_error &error) {
                non_targetable.push_back(target);
            }
        }
        UISystem::enableHighlights(targetable, 3, true, non_targetable);
    } else {
        UISystem::enableHighlights(targets, 3);
    }
}

// Check targetTile is within the world grid before calling this function!
void Player::on_click_tile(vec2 targetTile) {
    UISystem::disableHighlights();
    auto& turn = ECS::registry<Turn>.get(entity);
    TilePosition& tile_position = ECS::registry<TilePosition>.get(entity);
    bool can_move = turn.movement > 0;
    bool can_attack = turn.attacks > 0;

    vec2 character_tile = tile_position.grid_pos;
    vec2 target_tile = targetTile;

    if (moving && can_move) {// & (turn.currentAction == turn.IDLE) & !ECS::registry<Moving>.has(entity)) {
        // has targeted a tile for movement
        if (thePath.empty()) { // try finding path once
            findPath(character_tile, target_tile, turn.movement);
        }
        if (thePath.empty()) { // if still empty, movement is impossible
//            moving = false;
            turn.currentAction = turn.IDLE;
            return;
        }
        hasPath = true;
    } else if (attacking && can_attack) {
        // can't target own tile
        if (world->isOccupied(character_tile, target_tile)) return;
        if (ranged) {
            turn.attacks --;
            Boomerang::createBoomerang("normal", 0.4f, 3000, tile_position.grid_pos,
                                       target_tile, .08f, entity);
            attacked = true;
        } else {
            if(Player::inMeleeRange(character_tile, target_tile)) {
                world->attack_target(entity, target_tile, true);
                attacked = true;
            }
        }
        attackDir = target_tile - character_tile;
        UISystem::updateUI();
//        attacking = false;
        UISystem::updateButtons();
    } else if (casting) {  // unlimited casting for testing.
    // } else if (casting && can_attack) {
        // can't target own tile

        // TODO allowed??
        if (world->isOccupied(character_tile, target_tile)) return;
        Character& dude = ECS::registry<Character>.get(entity);
        Turn& turn = ECS::registry<Turn>.get(entity);
        auto spell = *spell_map[selected_spell];
        if(turn.spells >= spell.cost) {
            if(dude.castSpell(entity, spell, targetTile))
                // if cast success, subtract available casts by 1
                turn.spells -= spell.cost;
            else
                std::cout << "no more spells!\n";
        }

        // no animation.
        // attackDir = target_tile - character_tile;
        UISystem::updateUI();
//        casting = false;
//        selected_spell = WorldSystem::SpellButton::NONE;
        UISystem::updateButtons();
    }
}

void Player::move() {
    //assert(!thePath.empty());
    auto& turn = ECS::registry<Turn>.get(entity);
    TilePosition& tile_position = ECS::registry<TilePosition>.get(entity);
    if (thePath.empty()) {
//        moving = false;
        hasPath = false;
//        casting = false;
        show_tiles();
        UISystem::updateButtons();
        return;
    }
    if (turn.movement <= 0) {
        Player::clearPath();
//        hasPath = false;
        moving = false;
//        casting = false;
        show_tiles();
        UISystem::updateButtons();
        return;
    }
    if (!ECS::registry<Moving>.has(entity)) {
        vec2 destPos = thePath.front().vertex;
        thePath.erase(thePath.begin());
        world->move_character(turn, tile_position, destPos);
    }
}

void Player::on_click_move() {
    // TODO: grey out buttons when player cannot move
    if (ECS::registry<Player>.has(entity) && ECS::registry<Turn>.has(entity)) {
        auto &turn = ECS::registry<Turn>.get(entity);
        bool can_move = turn.movement > 0;
        moving = true;
        if (moving) {
            ranged = false;
            attacking = false;
            casting = false;
        }
        UISystem::updateButtons();
        if (can_move & !ECS::registry<Moving>.has(entity)) {
            turn.currentAction = turn.IDLE;
        }
    }
}

void Player::on_click_attack() {
    // TODO: grey out buttons when player cannot attack

    UISystem::updateButtons();
    if (ECS::registry<Player>.has(entity) && ECS::registry<Turn>.has(entity))
    {
        auto& turn = ECS::registry<Turn>.get(entity);
        moving = false;
        if (moving) {
            ranged = false;
            attacking = false;
            casting = false;
        }
        UISystem::updateButtons();
        if ((turn.currentAction == turn.IDLE) && turn.attacks > 0) {
            turn.currentAction = turn.ATTACKING;
            // TODO: implement ranged attack mode
            // selected_tile = {tile_position.grid_pos.x, tile_position.grid_pos.y};
        }
    }
}

void Player::on_click_end_turn() {
    if (hasPath) return;
    if (!ECS::registry<Projectile>.components.empty() || !ECS::registry<Boomerang>.components.empty()) return;
    UISystem::disableHighlights();
    clearPath();
    attacking = false;
    ranged = false;
    moving = false;
    UISystem::updateButtons();
    world->step_turn();
}

void Player::on_click_melee_attack() {
    attacking = true;
    if (attacking) {
        ranged = false;
        moving = false;
        casting = false;
    }
    UISystem::updateButtons();
    // ranged_attack_mode = false;
}

void Player::on_click_ranged_attack() {
    attacking = true;
    if (attacking) {
        ranged = true;
        moving = false;
        casting = false;
    }
    UISystem::updateButtons();
}

// testspell
void Player::on_click_spell_cast(WorldSystem::SpellButton button) {
    casting = true;
    if (casting) {
        attacking = false;
        ranged = false;
        moving = false;
        casting = true;
        selected_spell = button;
    }
    UISystem::updateButtons();
    // ranged_attack_mode = false;
}

void Player::findPath(vec2 gridPos, vec2 targetTile, int depth, bool ignoreOccupy) {
    if (isOccupied(targetTile) && !ignoreOccupy) {
        std::cout<< "failed to find path to " << targetTile.x << "," << targetTile.y << std::endl;
        return;
    }
    auto paths = std::vector<vec2>();
    vec2 goalPos = targetTile;

    std::queue<vec2> q2do = std::queue<vec2>();

    processed.clear();

    q2do.push(gridPos);

    // add first node with nullptr (so we know when to stop when backtracing)
    processed.emplace_back(new node_and_edge{vec2(q2do.front()), nullptr, 0});

    while(!q2do.empty()) {
        // get the next node
        auto curNode = new vec2(q2do.front());
        q2do.pop();

        //check tile above. Break if next to player
        auto nextNode = vec2(curNode->x, curNode->y-1);
        if(processCell(curNode, nextNode, goalPos, q2do, depth)) return;

        //check tile to the right. Break if next to player
        nextNode = vec2(curNode->x+1, curNode->y);
        if(processCell(curNode, nextNode, goalPos, q2do, depth)) return;

        //check tile below. Break if next to player
        nextNode = vec2(curNode->x, curNode->y+1);
        if(processCell(curNode, nextNode, goalPos, q2do, depth)) return;

        //check tile to the left. Break if next to player
        nextNode = vec2(curNode->x-1, curNode->y);
        if(processCell(curNode, nextNode, goalPos, q2do, depth)) return;
    }
}

// processes a cell. Returns true if the cell is the player cell.
// false otherwise.
bool Player::processCell(vec2* curNode, vec2 cell, vec2 goalPos, std::queue<vec2> & q2do, int depth) {
//    std::cout << "processing cell " << cell.x << ", " << cell.y << "\n";
    if(curNode->x == goalPos.x && curNode->y == goalPos.y) {
        Player::setPath(*curNode);
        std::cout << "found path\n";
        return true;
    }
    else if(!isOccupied(cell)) {

        // make sure it hasn't already been processed
        for(auto & i : processed) {
            if(i->vertex.x == cell.x && i->vertex.y == cell.y) {
                return false;
            }
        }

        node_and_edge * prev;
        for(auto & i : processed) {
            if(i->vertex.x == curNode->x && i->vertex.y == curNode->y) {
                prev = i;
                break;
            }
        }
        if (prev->depth >= depth) {
            return false;
        }
        // place current edge into the list of processed nodes and add nextNode (cell) to q2do
        q2do.push(cell);
        auto newOne = new node_and_edge(cell, prev, prev->depth + 1);
        processed.emplace_back(newOne);
    }
    return false;
}

// checks whether a cell is occupied
bool Player::isOccupied(vec2 cell) {
    if(WorldSystem::is_wall(cell) || WorldSystem::is_hole(cell)) {
//        std::cout << cell.x << ", " << cell.y << " is wall\n";
        return true;
    }
    for (auto entity : ECS::registry<TilePosition>.entities) {
        TilePosition& entity_pos = ECS::registry<TilePosition>.get(entity);
        if (entity_pos.grid_pos.x == cell.x && entity_pos.grid_pos.y == cell.y) {
            if (ECS::registry<Tile>.has(entity)) {
                auto& tile = ECS::registry<Tile>.get(entity);
                if (tile.isBlocking == 1) {
//                    std::cout << cell.x << ", " << cell.y << " is a blocking tile\n";
                    return true;
                }
            } else {
                if (!entity_pos.isSolid) return false;
//                std::cout << cell.x << ", " << cell.y << " is a blocking character\n";
                return true;
            }
        }
    }
    return false;
}

void Player::setPath(vec2 goal) {
    node_and_edge * cur_node;

    for(auto & i : processed) {
        if(i->vertex.x == goal.x && i->vertex.y == goal.y) {
            cur_node = i;
            break;
        }
    }

    // if we get here, the path should be empty.
    assert(thePath.empty());

    while(cur_node->link != NULL && cur_node->link != nullptr) {
        thePath.emplace(thePath.begin(), *cur_node);
        cur_node = cur_node->link;
    }
}

void Player::clearPath() {
    thePath.clear();
    for(auto & i : processed) {
        free(i);
    }
    processed.clear();
}

// perhaps in future add extra range for longer weapons here
bool Player::inMeleeRange(vec2 charPos, vec2 targetPos) {
    return ((abs(charPos.x - targetPos.x) == 1 && charPos.y == targetPos.y) ||
            (abs(charPos.y - targetPos.y) == 1 && charPos.x == targetPos.x));
}

void Player::show_move_tiles(vec2 playerPos, int movement) {
    moveable_tiles.clear();
    if (movement <= 0) return;
    clearPath();
    findPath(playerPos,{-1, -1},movement, true);
    for (auto& i : processed) {
        moveable_tiles.push_back(vec2(i->vertex));
    }
    clearPath();
    UISystem::enableHighlights(moveable_tiles, 1);
    return;
}

void Player::show_ranged_tiles(vec2 playerPos, int range) {
    int x = world->world_grid.size();
    int y = world->world_grid[0].size();
    std::vector<vec2> targetable_tiles;
    for(int i = 0; i < x; i++) {
        for (int j = 0; j < y; j++) {
            targetable_tiles.push_back(vec2(i,j));
        }
    }
    UISystem::enableHighlights(targetable_tiles, 2);
}
