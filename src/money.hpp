#include "common.hpp"
#include "tiny_ecs.hpp"
#include <functional>
#include "world.hpp"


class Money {
public:
    std::string name;
    int value;

    static ECS::Entity createMoney(const std::string& money_name, vec2 gridPos, int value);
};