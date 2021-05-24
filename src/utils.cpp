//
// Created by Roark on 2021-03-12.
//

#include "world.hpp"
#include "utils.hpp"
#include "inventory.hpp"
#include <utility>
#include "../ext/json/single_include/nlohmann/json.hpp"
#include "player.hpp"

using jsonf = nlohmann::json;
using json = nlohmann::json;
#include <fstream>


// Finds bounding edges of wall tiles in the map

void WorldSystem::GenerateBoundingEdges() {

    if (ECS::registry<Edge>.components.size()>0) {
        ECS::registry<Edge>.clear();
    }
    if (ECS::registry<V>.components.size()>0) {
        ECS::registry<V>.clear();
    }

    auto grid = WorldSystem::world_grid;

    const int width = grid.size();
    const int height = grid[0].size();

    // Cell_Edges edgegrid[width][height];
    std::vector< std::vector<Cell_Edges>>edgegrid(width, (std::vector<Cell_Edges>(height)));
    // set all to 0
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            auto &cur = edgegrid[x][y];

            for (int i = 0; i < 4; i++) {
                cur.has_edge[i] = false;
                cur.edge_ids[i] = -1;
            }
        }
    }

    std::map<int, Edge> pool;
    int cur_id = 0;

    // iterate through world grid once
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            auto &cur = edgegrid[x][y];

            // do nothing if it's empty
            if (grid[x][y] != Tile::WALL) continue;

            // North
            if (y == 0 || grid[x][y-1] == Tile::WALL) {
                // no edge here; pass
            } else if (x > 0) {
                // check left cell for an edge
                auto &left = edgegrid[x-1][y].edge_ids;
                if (left[0] != -1) {
                    // extend existing edge
                    pool.at(left[0]).end = vec2(x + 0.5, y - 0.5);
                    cur.edge_ids[0] = left[0];
                } else {
                    // insert an edge
                    pool.insert({cur_id, Edge({x - 0.5, y - 0.5}, {x + 0.5, y - 0.5})});
                    cur.edge_ids[0] = cur_id++; // make sure next edge has a different id
                }
            } else {
                // insert an edge
                pool.insert({cur_id, Edge({x - 0.5, y - 0.5}, {x + 0.5, y - 0.5})});
                cur.edge_ids[0] = cur_id++; // make sure next edge has a different id
            }

            // East
            if (x == width-1 || grid[x + 1][y] == Tile::WALL) {
                // For the east edges of wall face tiles
                // x != width - 1 ensures is not boundary of map,
                // grid[x + 1][y + 1] != Tile::WALL, makes sure that there is a north-facing wall on the right
                if (x < width-1 && y < height - 1 && grid[x + 1][y + 1] != Tile::WALL && grid[x][y + 1] == Tile::WALL) {
                    // insert an edge
                    pool.insert({cur_id, Edge({x + 0.5, y - 0.25}, {x + 0.5, y + 0.5})});
                    cur.edge_ids[1] = cur_id++; // make sure next edge has a different id
                }
                // else no edge here; pass
            } else if (y > 0) {
                // check upper cell for an edge
                auto &upper = edgegrid[x][y - 1].edge_ids;
                if (upper[1] != -1 && !pool.at(upper[1]).is_3d) {
                    if (y < height - 1 && grid[x][y + 1] != Tile::WALL) {
                        // extend existing edge
                        pool.at(upper[1]).end = vec2(x + 0.5, y - 0.25);
                        cur.edge_ids[1] = upper[1];
                        // wall face tile
                        pool.insert({cur_id, Edge({x + 0.5, y - 0.25}, {x + 0.5, y + 0.5}, true)});
                        cur.edge_ids[1] = cur_id++; // make sure next edge has a different id
                    } else {
                        // extend existing edge
                        pool.at(upper[1]).end = vec2(x + 0.5, y + 0.5);
                        cur.edge_ids[1] = upper[1];
                    }
                } else {
                    // insert an edge
                    if (y < height - 1 && grid[x][y + 1] != Tile::WALL) {
                        // extend existing edge
                        pool.insert({cur_id, Edge({x + 0.5, y - 0.5}, {x + 0.5, y - 0.25})});
                        cur.edge_ids[1] = cur_id++;
                        // wall face tile
                        pool.insert({cur_id, Edge({x + 0.5, y - 0.25}, {x + 0.5, y + 0.5}, true)});
                        cur.edge_ids[1] = cur_id++; // make sure next edge has a different id
                    } else {
                        pool.insert({cur_id, Edge({x + 0.5, y - 0.5}, {x + 0.5, y + 0.5})});
                        cur.edge_ids[1] = cur_id++; // make sure next edge has a different id
                    }
                }
            } else {
                // For y == 0 (top right)
                // insert an edge
                pool.insert({cur_id, Edge({x + 0.5, y - 0.5}, {x + 0.5, y + 0.5})});
                cur.edge_ids[1] = cur_id++; // make sure next edge has a different id
            }

            // South
            if (y == height-1 || grid[x][y+1] == Tile::WALL) {
                // no edge here; pass
            } else if (x > 0) {
                // check left cell for an edge
                auto &left = edgegrid[x-1][y].edge_ids;
                if (left[2] != -1) {
                    // extend existing edge
                    pool.at(left[2]).end = vec2(x + 0.5, y - 0.25);
                    cur.edge_ids[2] = left[2];
                } else {
                    // insert an edge
                    pool.insert({cur_id, Edge({x - 0.5, y - 0.25}, {x + 0.5, y - 0.25})});
                    cur.edge_ids[2] = cur_id++; // make sure next edge has a different id
                }
            } else {
                // insert an edge
                pool.insert({cur_id, Edge({x - 0.5, y - 0.25}, {x + 0.5, y - 0.25})});
                cur.edge_ids[2] = cur_id++; // make sure next edge has a different id
            }

            // West
            if (x == 0 || grid[x - 1][y] == Tile::WALL) {
                // For the west edges of wall face tiles
                if (x > 0 && y < height - 1 && grid[x - 1][y + 1] != Tile::WALL  && grid[x][y + 1] == Tile::WALL) {
                    // insert an edge
                    pool.insert({cur_id, Edge({x - 0.5, y - 0.25}, {x - 0.5, y + 0.5})});
                    cur.edge_ids[3] = cur_id++; // make sure next edge has a different id
                }
                // else no edge here; pass
            } else if (y > 0) {
                // check lower cell for an edge
                auto &upper = edgegrid[x][y - 1].edge_ids;
                if (upper[3] != -1  && !pool.at(upper[3]).is_3d) {
                    if (y < height - 1 && grid[x][y + 1] != Tile::WALL) {
                        // extend existing edge
                        pool.at(upper[3]).end = vec2(x - 0.5, y - 0.25);
                        cur.edge_ids[3] = upper[1];
                        // wall face tile
                        pool.insert({cur_id, Edge({x - 0.5, y - 0.25}, {x - 0.5, y + 0.5}, true)});
                        cur.edge_ids[3] = cur_id++; // make sure next edge has a different id
                    } else {
                        // extend existing edge
                        pool.at(upper[3]).end = vec2(x - 0.5, y + 0.5);
//                    pool.at(upper[3]).is_3d = false;
                        cur.edge_ids[3] = upper[3];
                    }
                } else {
                    // insert an edge
                    if (y < height - 1 && grid[x][y + 1] != Tile::WALL) {
                        // extend existing edge
                        pool.insert({cur_id, Edge({x - 0.5, y - 0.5}, {x - 0.5, y - 0.25})});
                        cur.edge_ids[3] = cur_id++;
                        // wall face tile
                        pool.insert({cur_id, Edge({x - 0.5, y - 0.25}, {x - 0.5, y + 0.5}, true)});
                        cur.edge_ids[3] = cur_id++; // make sure next edge has a different id
                    } else {
                        pool.insert({cur_id, Edge({x - 0.5, y - 0.5}, {x - 0.5, y + 0.5})});
                        cur.edge_ids[3] = cur_id++; // make sure next edge has a different id
                    }
                }
            } else {
                // For top left
                // insert an edge
                pool.insert({cur_id, Edge({x - 0.5, y - 0.5}, {x - 0.5, y + 0.5})});
                cur.edge_ids[3] = cur_id++; // make sure next edge has a different id
            }
        }
    }

    ECS::Entity entity = ECS::Entity();
    for (int i = 0; i < cur_id; i++) {
        auto& edge = pool.at(i);
//        std::cout << "edge " << i << ". s: " << edge.start.x << ", " << edge.start.y
//                  << "; e: " << edge.end.x << ", " << edge.end.y << "\n";

        // emplace edge
        ECS::registry<Edge>.emplace_with_duplicates(entity, edge);
    }

    vec2 tl = vec2(-0.5, -0.5);
    vec2 tr = vec2(width-0.5, -0.5);
    vec2 bl = vec2(-0.5, height-0.5);
    vec2 br = vec2(width-0.5, height-0.5);
    ECS::registry<Edge>.emplace_with_duplicates(entity, Edge(tl, tr));
    ECS::registry<Edge>.emplace_with_duplicates(entity, Edge(tr, br));
    ECS::registry<Edge>.emplace_with_duplicates(entity, Edge(br, bl));
    ECS::registry<Edge>.emplace_with_duplicates(entity, Edge(bl, tl));

    for(auto& edge : ECS::registry<Edge>.components) {

        // emplace vertices without duplicates
        if(ECS::registry<V>.size() > 0) {
            bool exist[2] = {false, false};
            for(auto& v : ECS::registry<V>.components) {
                if(v.pos.x == edge.start.x && v.pos.y == edge.start.y)
                    exist[0] = true;
                if(v.pos.x == edge.end.x && v.pos.y == edge.end.y)
                    exist[1] = true;
            }

            if(!exist[0]) {
                auto& v = ECS::registry<V>.emplace_with_duplicates(entity);
                v.pos = edge.start;
            }

            if(!exist[1]) {
                auto& v = ECS::registry<V>.emplace_with_duplicates(entity);
                v.pos = (edge.end);
            }
        } else {
            auto& v1 = ECS::registry<V>.emplace_with_duplicates(entity);
            v1.pos = edge.start;
            auto& v2 = ECS::registry<V>.emplace_with_duplicates(entity);
            v2.pos = edge.end;
        }
        edge.x3 = edge.start.x;
        edge.y3 = edge.start.y;
        edge.x4 = edge.end.x-edge.start.x;
        edge.y4 = edge.end.y-edge.start.y;
    }
}

void WorldSystem::load_inventory(ECS::Entity player) {
    auto& inventory = ECS::registry<Inventory>.emplace(player);
    std::ifstream json_obj(save_path("player_inventory.json"));
    std::string content((std::istreambuf_iterator<char>(json_obj)),
                        (std::istreambuf_iterator<char>()));
    if (json_obj.is_open()) {
        auto inv = json::parse(content);
        // Parse JSON
        for (auto& element : inv.items()) {
            // Items
            if (element.key() == "weapons") {
                for (auto& i : element.value()) {
                    auto name = i.find("name");
                    auto damage = i.find("damage");
                    inventory.weapons.emplace_back(Weapon(name.value(), damage.value()));
                    auto &w = inventory.weapons.back();
                }
            } else
            if (element.key() == "artifacts") {
                for (auto& i : element.value()) {
                    inventory.artifacts.emplace_back(Artifact());
                    auto &a = inventory.artifacts.back();
                    auto area = i.find("area");
                    a.area = area.value();
                    auto name = i.find("name");
                    a.name = name.value();
                }
            } else
            if (element.key() == "gold") {
                inventory.gold = element.value();
            } else
            if (element.key() == "ap") {
                inventory.ap = element.value();
            } else
            if (element.key() == "sp") {
                inventory.sp = element.value();
            } else
            if (element.key() == "hp") {
                inventory.hp = element.value();
            } else
            if (element.key() == "mp") {
                inventory.mp = element.value();
            } else
            if (element.key() == "spells") {
                auto& p = ECS::registry<Player>.get(player);
                for (auto& i : element.value()) {
                    auto button = i.find("spell_button").value();
                    auto spellEffect = i.find("spell").value();
                    inventory.spell_map[button] = spellEffect;
                    p.modify_spell_map(button, spellEffect);
                    updateSpellButton(button, spellEffect);
                }
            }
        }
        json_obj.close();   //close the file object.
    }
    std::cout << "Loaded inventory.\n";
}

void WorldSystem::save_inventory(Inventory inv) {
    jsonf save;
    json artifacts = json::array();
    for (auto& a : inv.artifacts) {
        artifacts.push_back({{"name", a.name}, {"area", a.area}});
    }
    json weapons = json::array();
    for (auto& w : inv.weapons) {
        weapons.push_back({{"name", w.name}, {"damage", w.damage}});
    }
    json spells = json::array();
    for (auto& spell_pair : inv.spell_map) {
        spells.push_back({{"spell_button", spell_pair.first},{"spell", spell_pair.second}});
    }

    save["gold"] = inv.gold;

    save["artifacts"] = artifacts;
    save["weapons"] = weapons;
    save["spells"] = spells;
    save["hp"] = inv.hp;
    save["sp"] = inv.sp;
    save["ap"] = inv.ap;
    save["mp"] = inv.mp;

    std::ofstream savefile(save_path("player_inventory.json"));
    if (savefile.is_open()) {
        savefile << save;
        savefile.close();
    }
    else {
        std::cout << "Can't save inventory!\n";
    }
}
