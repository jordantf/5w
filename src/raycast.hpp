//
// Created by Roark on 2021-04-11.
//
#pragma once
#include "render.hpp"
vec2 RenderSystem::rayCastQ1(vec2 o, vec2 d, float max_dist) {
    float x1 = o.x;     float y1 = o.y;
    float x2 = d.x-o.x; float y2 = d.y-o.y;
    float slope = y2/x2;
    float yslope = x2/y2;

    vec2 worldSize = {WorldSystem::world_grid.size(), WorldSystem::world_grid[0].size()};

    bool inWall = WorldSystem:: world_grid[floor(o.x)][floor(o.y)] == Tile::WALL;

    // find first x intersection
    float quantize_x = round(x1) + 0.5f;
    float dx = quantize_x - x1;
    vec2 xsect = { x1+dx, y1+dx*slope };
    bool foundX = false;

    if(xsect.y > -0.5 && xsect.x > 0 && xsect.y < worldSize.y - 1 && xsect.x < worldSize.x - 1) {
        int w = (int) lround(xsect.x + 0.5);
        if (WorldSystem::world_grid[w][(int) lround(xsect.y)] != Tile::WALL
        || ((xsect.y - (round(xsect.y)-0.5)) > 0.25 && WorldSystem::world_grid_tileset[w][(int) lround(xsect.y)] < 12)) {
            // if the first one wasn't a wall, continue stepping

            // find subsequent intersections
            while (!foundX) {
                xsect.x++; xsect.y += slope;
                if (xsect.x < 0 || xsect.x >= worldSize.x-1) break;
                if (xsect.y < -0.5 || xsect.y >= worldSize.y-1) break;
                if (xsect.x > 50 || xsect.y > 50) break;
                // check tile depending on direction of ray
                w = (int) lround(xsect.x + 0.5);
                if(WorldSystem::world_grid[w][(int) lround(xsect.y)] == Tile::WALL) {
                    if (WorldSystem::world_grid_tileset[w][(int) lround(xsect.y)] > 11) foundX = true;
                    else if ((xsect.y - (round(xsect.y)-0.5)) < 0.25) foundX = true;
                }
            }
        }
    }

    // find first y intersection
    float quantize_y = round(y1 ) + 0.5f; // Going down
    float dy = quantize_y - y1;
    vec2 ysect = {x1+dy*yslope, y1+dy};
    bool foundY = false;

    if(ysect.y > -0.5 && ysect.x > 0 && ysect.y < worldSize.y - 1 && ysect.x < worldSize.x - 1) {
        int w = (int) lround(ysect.y + 0.5);
        if (WorldSystem::world_grid[(int) lround(ysect.x)][w] != Tile::WALL) {
            // if the first one wasn't a wall, continue stepping

            // find subsequent intersections
            while (!foundY) {
                ysect.y++; ysect.x += yslope;

                if (ysect.x < 0 || ysect.x >= worldSize.x-1) break;
                if (ysect.y < -0.5 || ysect.y >= worldSize.y-1) break;
                if (ysect.x > 50 || ysect.y > 50) break;

                w = (int) lround(ysect.y + 0.5);
                foundY = WorldSystem::world_grid[(int) lround(ysect.x)][w] == Tile::WALL;
            }
        }
    }

    // for now, assume an intersection is always found
    if(xsect.x < ysect.x) return xsect;
    if(ysect.x < xsect.x) return ysect;
    return {0,0};
}

//vec2 RenderSystem::rayCastQ1(vec2 o, vec2 d, float max_dist) {
//    float x1 = o.x;
//    float y1 = o.y;
//    float x2 = d.x-o.x;
//    float y2 = d.y-o.y;
//    vec2 best = {-1, -1};
//    float bestdist = 10000;
//    float epsilon = 0.0000001;
//
//    for(auto& edge : ECS::registry<Edge>.components) {
//        vec2 es = edge.start;
//        vec2 ee = edge.end;
//
//        if (edge.is_3d) {
//            continue;
//        }
//
//        float x3 = es.x;
//        float y3 = es.y;
//        float x4 = ee.x-es.x;
//        float y4 = ee.y-es.y;
//
//        float T2 = (x2*(y3-y1) + y2*(x1-x3))/(x4*y2 - y4*x2);
//        float T1 = (x3+x4*T2-x1)/x2;
//        if(T1 <= 0-epsilon) continue;
//        if(T2 < 0-epsilon || T2 > 1+epsilon) continue;
//
//        vec2 intersection = vec2(x1+x2*T1, y1+y2*T1);
//
//        if(best.x == -1) {
//            best = intersection;
//            bestdist = T1;
//        } else {
//
//            if(T1 < bestdist) {
//                bestdist = T1;
//                best = intersection;
//            }
//        }
//    }
//    return best;
//}

vec2 RenderSystem::rayCastQ2(vec2 o, vec2 d, float max_dist) {
    float x1 = o.x;     float y1 = o.y;
    float x2 = d.x-o.x; float y2 = d.y-o.y;
    float slope = y2/x2;
    float yslope = x2/y2;

    vec2 worldSize = {WorldSystem::world_grid.size(), WorldSystem::world_grid[0].size()};

    // find first x intersection
    float quantize_x = round(x1) - 0.5f;
    float dx = quantize_x - x1;
    vec2 xsect = { x1+dx, y1+dx*slope };
    bool foundX = false;

    if(xsect.y > -0.5 && xsect.x > 1 && xsect.y < worldSize.y - 1 && xsect.x < worldSize.x - 1) {
        int w = (int) lround(xsect.x - 0.5);
        if (WorldSystem::world_grid[w][(int) lround(xsect.y)] != Tile::WALL
            || ((xsect.y - (round(xsect.y)-0.5)) > 0.25 && WorldSystem::world_grid_tileset[w][(int) lround(xsect.y)] < 12)) {
            // if the first one wasn't a wall, continue stepping

            // find subsequent intersections
            while (!foundX) {
                xsect.x--; xsect.y -= slope;
                if (xsect.x < 0 || xsect.x >= worldSize.x-1) break;
                if (xsect.y < -0.5 || xsect.y >= worldSize.y-1) break;
                if (xsect.x > 50 || xsect.y > 50) break;
                // check tile depending on direction of ray
                w = (int) lround(xsect.x - 0.5);
                if(WorldSystem::world_grid[w][(int) lround(xsect.y)] == Tile::WALL) {
                    if (WorldSystem::world_grid_tileset[w][(int) lround(xsect.y)] > 11) foundX = true;
                    else if ((xsect.y - (round(xsect.y)-0.5)) < 0.25) foundX = true;
                }
            }
        }
    }

    // find first y intersection
    float quantize_y = round(y1) + 0.5f; // Going down
    float dy = quantize_y - y1;
    vec2 ysect = {x1+dy*yslope, y1+dy};
    bool foundY = false;

    if(ysect.y > -0.5 && ysect.x > 0 && ysect.y < worldSize.y - 1 && ysect.x < worldSize.x - 1) {
        int w = (int) lround(ysect.y + 0.5);
        if (WorldSystem::world_grid[(int) lround(ysect.x)][w] != Tile::WALL) {
            // if the first one wasn't a wall, continue stepping

            // find subsequent intersections
            while (!foundY) {
                ysect.y++;
                ysect.x += yslope;

                if (ysect.x < 0 || ysect.x >= worldSize.x - 1) break;
                if (ysect.y < -0.5 || ysect.y >= worldSize.y - 1) break;
                if (ysect.x > 50 || ysect.y > 50) break;

                w = (int) lround(ysect.y + 0.5);
                foundY = WorldSystem::world_grid[(int) lround(ysect.x)][w] == Tile::WALL;
            }
        }
    }

    // for now, assume an intersection is always found
    if(xsect.x > ysect.x) return xsect;
    if(ysect.x > xsect.x) return ysect;
    return {0,0};
}

vec2 RenderSystem::rayCastQ3(vec2 o, vec2 d, float max_dist) {
    float x1 = o.x;     float y1 = o.y;
    float x2 = d.x-o.x; float y2 = d.y-o.y;
    float slope = y2/x2; // positive
    float yslope = x2/y2; // positive

    if (WorldSystem:: world_grid[floor(o.x)][floor(o.y)] == Tile::WALL) {
        if (slope > 0.5) {
            float y = -0.25;
            return o + vec2(yslope * y,y);
        }
    }

    vec2 worldSize = {WorldSystem::world_grid.size(), WorldSystem::world_grid[0].size()};

    // find first x intersection
    float quantize_x = round(x1) - 0.5f;
    float dx = quantize_x - x1; // negative
    vec2 xsect = { x1+dx, y1+dx*slope };
    bool foundX = false;

    if(xsect.y >= -0.4 && xsect.x >= 0 && xsect.y < worldSize.y - 1 && xsect.x < worldSize.x - 1) {
        int w = (int) lround(xsect.x - 0.5);
        if (WorldSystem::world_grid[w][(int) lround(xsect.y)] == Tile::WALL) {
            if(WorldSystem::world_grid_tileset[w][(int) lround(xsect.y)] > 11) {
                foundX = true;
            } else if ((xsect.y - (lround(xsect.y) - 0.5)) < 0.25) {
                // case 1: check if we hit the side of thin wall
                foundX = true;
            } else if (((lround(xsect.y) + 0.5) - (xsect.y - slope)) > 0.75) {
                // case 2: check if we hit the front of the thin wall
                foundX = true;
                // set new intersection on thin wall
                float dy = xsect.y - (round(xsect.y) - 0.25);
                xsect.y = lround(xsect.y) - 0.25;
                xsect.x -= dy * yslope;
            }
        }
        if(!foundX) {

            // if the first one wasn't a wall, continue stepping

            // find subsequent intersections
            while (!foundX) {
                xsect.x--; xsect.y -= slope;
                if (xsect.x < 0 || xsect.x >= worldSize.x-1) break;
                if (xsect.y < -0.4 || xsect.y >= worldSize.y-1) break;
                if (xsect.x > 50 || xsect.y > 50) break;
                // check tile depending on direction of ray
                w = (int) lround(xsect.x - 0.5);
                if(WorldSystem::world_grid[w][(int) lround(xsect.y)] == Tile::WALL) {
                    if(WorldSystem::world_grid_tileset[w][(int) lround(xsect.y)] > 11) {
                        foundX = true;
                    } else if ((xsect.y - (lround(xsect.y) - 0.5)) < 0.25) {
                        // case 1: check if we hit the side of thin wall
                        foundX = true;
                    } else if (((lround(xsect.y) + 0.5) - (xsect.y - slope)) > 0.75) {
                        // case 2: check if we hit the front of the thin wall
                        foundX = true;
                        // set new intersection on thin wall
                        float dy = xsect.y - (round(xsect.y) - 0.25);
                        xsect.y = lround(xsect.y) - 0.25;
                        xsect.x -= dy * yslope;
                    }
                }
            }
        }
    }

    // find first y intersection
    float quantize_y = round(y1) - 0.5f; // Going up
    float dy = quantize_y - y1; // negative
    vec2 ysect = {x1+dy*yslope, y1+dy};
    bool foundY = false;

    if(ysect.y >= -0.4 && ysect.x >= 0 && ysect.y < worldSize.y - 1 && ysect.x < worldSize.x - 1) {
        int w = (int) lround(ysect.y - 0.5);
        if (WorldSystem::world_grid[(int) lround(ysect.x)][w] == Tile::WALL) {
            auto tile = WorldSystem::world_grid_tileset[(int) lround(ysect.x)][w];
            if(tile > 11) {
                foundY = true;
            } else if (tile < 18) {
                // case 1: see if we hit the wall
                float dy = -0.75;
                float newX = ysect.x + dy * yslope;
                if(newX > (round(ysect.x) - 0.5)) {
                    // we do touch the wall
                    foundY = true;
                    ysect.y += dy; // go to -0.25;
                    ysect.x = newX;
                }
                // else we don't touch the wall; skip
            }
        }

        if(!foundY) {
            // if the first one wasn't a wall, continue stepping

            // find subsequent intersections
            while (!foundY) {
                ysect.y--; ysect.x -= yslope;

                if (ysect.x < 0 || ysect.x >= worldSize.x-1) break;
                if (ysect.y < -0.4 || ysect.y >= worldSize.y-1) break;
                if (ysect.x > 50 || ysect.y > 50) break;

                w = (int) lround(ysect.y - 0.5);
                if (w < 0) break;

                if(WorldSystem::world_grid[(int) lround(ysect.x)][w] == Tile::WALL) {
                    auto tile = WorldSystem::world_grid_tileset[(int) lround(ysect.x)][w];
                    if (tile > 11) {
                        foundY = true;
                    } else {
                        // case 1: see if we hit the wall
                        float dy = -0.75;
                        float newX = ysect.x + dy * yslope;
                        if (newX > (round(ysect.x) - 0.5)) {
                            // we do touch the wall
                            foundY = true;
                            ysect.y += dy; // go to -0.25;
                            ysect.x = newX;
                        }
                        // else we don't touch the wall; skip
                    }
                }
            }
        }
    }

    // for now, assume an intersection is always found
    if(xsect.x > ysect.x) return xsect;
    if(ysect.x > xsect.x) return ysect;
    return {0,0};
}

// case 1: it is a solid wall
// case 2: check if we hit the side of thin wall
// case 3: check if we hit front of thin wall

vec2 RenderSystem::rayCastQ4(vec2 o, vec2 d, float max_dist) {

    float x1 = o.x;     float y1 = o.y;
    float x2 = d.x-o.x; float y2 = d.y-o.y;
    float slope = y2/x2;
    float yslope = x2/y2;

    vec2 worldSize = {WorldSystem::world_grid.size(), WorldSystem::world_grid[0].size()};

    if (WorldSystem:: world_grid[floor(o.x)][floor(o.y)] == Tile::WALL) {
        if (slope < -0.5) {
            float y = -0.25;
            return o + vec2(yslope * y,y);
        }
    }

    // find first x intersection
    float quantize_x = round(x1) + 0.5f;
    float dx = quantize_x - x1;
    vec2 xsect = { x1+dx, y1+dx*slope };
    bool foundX = false;

    if(xsect.y > -0.4 && xsect.x > 0 && xsect.y < worldSize.y - 1 && xsect.x < worldSize.x - 1) {
        int w = (int) lround(xsect.x + 0.5);
        if (WorldSystem::world_grid[w][(int) lround(xsect.y)] == Tile::WALL) {
            if(WorldSystem::world_grid_tileset[w][(int) lround(xsect.y)] > 11) {
                foundX = true;
            } else if ((xsect.y - (lround(xsect.y) - 0.5)) < 0.25) {
                // case 2: check thin wall
                foundX = true;
            } else if (((lround(xsect.y) + 0.5) - (xsect.y + slope)) > 0.75) {
                // case 3: we hit the front of the wall THIS ONE IS BROKEN
                float dy = (round(xsect.y) - 0.25) - xsect.y;
                xsect.y = round(xsect.y) - 0.25;
                xsect.x += yslope * dy;
                if (DebugSystem::in_debug_mode)
                    DebugSystem::createLine(xsect * 60.f + 200.f, vec2(40,40));
                foundX = true;
            }
        }
        if(!foundX) {
            // if the first one wasn't a wall, continue stepping

            // find subsequent intersections
            while (!foundX) {
                xsect.x++; xsect.y += slope;
                if (xsect.x < 0 || xsect.x >= worldSize.x-1) break;
                if (xsect.y < -0.4 || xsect.y >= worldSize.y-1) break;
                if (xsect.x > 50 || xsect.y > 50) break;
                // check tile depending on direction of ray
                w = (int) lround(xsect.x + 0.5);
                if(WorldSystem::world_grid[w][(int) lround(xsect.y)] == Tile::WALL) {
                    if(WorldSystem::world_grid_tileset[w][(int) lround(xsect.y)] > 11) {
                        foundX = true;
                    } else if ((xsect.y - (lround(xsect.y) - 0.5)) < 0.25) {
                        // case 2: check thin wall
                        foundX = true;
                    }
                    else if (((lround(xsect.y) + 0.5) - (xsect.y + slope)) > 0.75) {
                        // case 3: we hit the front of the wall THIS ONE IS BROKEN
                        float dy = round(xsect.y) - 0.25 - xsect.y;
                        xsect.y = round(xsect.y) - 0.25;
                        xsect.x += yslope * dy;

                        if (DebugSystem::in_debug_mode)
                            DebugSystem::createLine(xsect * 60.f + 200.f, vec2(40,40));
                        foundX = true;
                    }
                }
            }
        }
    }

    // find first y intersection
    float quantize_y = round(y1) - 0.5f; // Going up
    float dy = quantize_y - y1;
    vec2 ysect = {x1+dy*yslope, y1+dy};
    bool foundY = false;

    if(ysect.y > -0.4 && ysect.x > 0 && ysect.y < worldSize.y-1 && ysect.x < worldSize.x-1) {
        int w = (int) lround(ysect.y - 0.5);
        if (WorldSystem::world_grid[(int) lround(ysect.x)][w] == Tile::WALL) {
            if(WorldSystem::world_grid_tileset[(int) lround(ysect.x)][w] > 11) {
                foundY = true;
            } else {
                // case 2: we hit the front.
                float dy = -0.75;
                float newX = ysect.x + dy * yslope;
                if((newX)<(round(ysect.x) + 0.5)) {
                    foundY = true;
                    ysect.y += dy;
                    ysect.x = newX;
                }
                // else we don't hit the front.
            }
        }
        if(!foundY) {
            // if the first one wasn't a wall, continue stepping

            // find subsequent intersections
            while (!foundY) {
                ysect.y--; ysect.x -= yslope;

                if (ysect.x < 0 || ysect.x >= worldSize.x-1) break;
                if (ysect.y < -0.4 || ysect.y >= worldSize.y-1) break;
                if (ysect.x > 50 || ysect.y > 50) break;

                w = (int) lround(ysect.y - 0.5);
                if (w < 0) break;

                if(WorldSystem::world_grid[(int) lround(ysect.x)][w] == Tile::WALL) {
                    if(WorldSystem::world_grid_tileset[(int) lround(ysect.x)][w] > 11) {
                        foundY = true;
                    } else {
                        // case 2: we hit the front.
                        float dy = -0.75;
                        float newX = ysect.x + dy * yslope;
                        if((newX)<(round(ysect.x) + 0.5)) {
                            foundY = true;
                            ysect.y += dy;
                            ysect.x = newX;
                        }
                        // else we don't hit the front.
                    }
                }
            }
        }
    }

    // for now, assume an intersection is always found
    if(xsect.x < ysect.x) return xsect;
    if(ysect.x < xsect.x) return ysect;
    return {0,0};
}

