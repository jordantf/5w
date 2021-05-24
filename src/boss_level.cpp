
#include "world.hpp"
#include "utils.hpp"
#include "inventory.hpp"
#include <utility>
#include "../ext/json/single_include/nlohmann/json.hpp"
#include "animSystem.hpp"
#include "BossAI.hpp"
#include <fstream>
#include "ui.hpp"


void WorldSystem::loadShopkeeper(bool bossmode) {
    unloadShopkeeper();
    if (bossmode) {
        ParticleSystem::createParticleBurst("particles/smoke.png",100, 0.f, 50.f,
                                            {0,0}, window_size, {1000.f, 3000.f},
                                            0, 2.0, {0.f, 10.f}, 0.93, 200.0, 600.f, 5.f,
                                            {0.3, 0.8}, 0.4);

        Key::createKey({6,8}, 5);

        for (auto e : ECS::registry<Torch>.entities) {
            Torch::removeTorch(e);
        }
        sound.play("bgm_area4");

        Animation* default_anim = setAnim("shopkeeper_1", "shopkeeper_1.png",
                                     { 16, 16 }, { 64, 16 },
                                     4, 12, true, false, false, nullptr);
        auto se = Character::createAnimatedCharacter("shopkeeper0", {23,7}, *default_anim, 4, 400, 5);
        ECS::registry<EnemyAIContainer>.emplace(se, new BossAI(se, this));
        ECS::registry<Shopkeeper>.emplace(se);
        ECS::registry<LightSource>.emplace(se);

        turn_queue.push_back(se);


        auto *skAnim = setAnim("ancient_skeleton_idle", "ancient_skeleton_idle.png", {16, 16},
                               {32, 16}, 2, 2, true, false, false, nullptr);
        spawn_character("ancient_skeleton0", {6,1}, 7, Animated, skAnim, 3, 60,
                        60, 5);

        auto *wraithAnim = setAnim("wraith_idle", "wraith_idle.png", {16, 16},
                                   {16, 32}, 2, 2, true, false, false, nullptr);
        spawn_character("wraith0", {15,1}, 6, Animated, wraithAnim, 6, 35,
                        35, 20);



        Torch::createTorch({23,5},true);
        Torch::createTorch({6,0},true);
        ECS::registry<TempText>.emplace(UISystem::showText("Ernest has stolen your artifacts!\nGet them back!", {100, 400}, 0.8f), 8000.f);

    } else {
        auto se = Character::createCharacter("shopkeeper", {6,4}, 0);
        ECS::registry<Character>.remove(se);
        ECS::registry<Shopkeeper>.emplace(se);
        auto& tile = ECS::registry<TilePosition>.get(se);

        sound.play("bgm_shop");

        auto p = Power::createPower("power_gold.png", "gold sword", 30, {6,5});
        ECS::registry<Power>.remove(p);
        auto& i = ECS::registry<Interactible>.insert(p, Interactible("Buy weapon\n800 Gold"));
        i.onInteract = [this,i,p](ECS::Entity* entity) {
            if (ECS::registry<Inventory>.has(*entity)) {
                auto& i = ECS::registry<Inventory>.get(*entity);
                if (i.gold >= 800) {
                    i.gold-= 800;
                    auto &c = ECS::registry<Character>.get(*entity);
                    c.meleeDamage = 30;
                    sound.play("reward");
                    std::cout << "player picked up POWER ITEM, new damage: " << 30 << std::endl;
                    ECS::registry<Weapon>.remove(*entity);
                    Weapon &w = ECS::registry<Weapon>.emplace(*entity, "gold sword", 30);
                    i.weapons.clear();
                    i.weapons.push_back(w);
                    UISystem::updateUI();
                    if (current_area == 4) this->loadShopkeeper(true);
                    auto& in = ECS::registry<Interactible>.get(p);
                    in.destroy();
                    ECS::registry<Interactible>.remove(p);
                    ECS::ContainerInterface::remove_all_components_of(p);
                } else {
                    ECS::registry<TempText>.emplace(UISystem::showText("You can't afford this!\nCome back when you're a \nlittle... mmm - richer.",
                                                                       {400, 300}, 0.4f), 8000.f);
                }
            }
        };
    }
}

void WorldSystem::unloadShopkeeper() {
    while (ECS::registry<Shopkeeper>.entities.size()>0)
        ECS::ContainerInterface::remove_all_components_of(ECS::registry<Shopkeeper>.entities.back());
}
