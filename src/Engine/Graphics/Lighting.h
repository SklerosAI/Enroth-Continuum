#pragma once

#include <span>
#include <vector>

struct BLVFace;
struct BLVLight;
struct LightsStack_StationaryLight_;
struct LightsStack_MobileLight_;
struct RenderBillboard;

extern LightsStack_StationaryLight_ *pStationaryLightsStack;
extern LightsStack_MobileLight_ *pMobileLightsStack;

void DrawLightsDebugOutlines(char bit_one_for_list1__bit_two_for_list2);
int _43F55F_get_billboard_light_level(const RenderBillboard *a1, int uBaseLightLevel);
int GetLightLevelAtPoint(unsigned int uBaseLightLevel, int uSectorID, float x, float y, float z);

/**
 * @param face                          Face of the loaded MM6 indoor map.
 * @return                              Dim level (0-31) that MM6 lights the face from before any lights are added,
 *                                      0 being full bright. It is the level of the face's own sector.
 */
int mm6IndoorFaceDimLevel(const BLVFace &face);

/**
 * @param sectorIds                     Sectors of the loaded indoor map.
 * @return                              The lights these sectors list that are switched on, each once.
 */
std::vector<const BLVLight *> litSectorLights(std::span<const int> sectorIds);
