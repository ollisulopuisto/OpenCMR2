#ifndef OPENCMR2_CURVED_ROAD_H
#define OPENCMR2_CURVED_ROAD_H
struct GenericFile;
void CurvedRoad_Begin(GenericFile *archive,const char *stage);
void *CurvedRoad_Render();
void *CurvedRoad_Contact();
#endif
