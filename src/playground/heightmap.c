#include "heightmap.h"

#include <SDL/SDL_image.h>
#include <err.h>
#include "global.h"

void load_heightmap(const char *path, Heightmap *hm) {
	SDL_Surface *surf = IMG_Load(path);
	
	hm->w = surf->w;
	hm->h = surf->h;
	
	hm->verts = (Vertex*) calloc(sizeof(Vector), hm->w*hm->h);
	hm->normals = (Vector*) calloc(sizeof(Vector), (hm->w-1)*hm->h*2);
		
	int x, y;
	for(y = 0; y < hm->h; y++) {
		for(x = 0; x < hm->w; x++) {
			Vertex* ptr = &hm->verts[y*hm->w+x];
			
			ptr->x = x*RESOLUTION;
			ptr->y = y*RESOLUTION - hm->h*RESOLUTION;
			ptr->z = ((Uint8 *)surf->pixels)[y*surf->pitch+x*3];
		}
	}
	
	// genarating normals
	for(y = 0; y < hm->h-1; y++) {
		for(x = 0; x < hm->w-1; x++) {
			Vertex v[] = {
				hm->verts[y*hm->w+x],
				hm->verts[y*hm->w+x+1],
				hm->verts[(y+1)*hm->w+x+1],
				hm->verts[(y+1)*hm->w+x]		
			};
			
			Vector vec[4];
			
			make_vector(vec+0, v+0, v+2); 
			make_vector(vec+1, v+0, v+1); 
			make_vector(vec+2, v+2, v+1); 
			make_vector(vec+3, v+2, v+3); 
			
			Vector* norm0 = &hm->normals[y*hm->w+x*2];
			Vector* norm1 = &hm->normals[y*hm->w+x*2+1];
			
			cross_product(norm0, &vec[0], &vec[1]);
			cross_product(norm1, &vec[2], &vec[3]);
			
			norm_vector(norm0);
			norm_vector(norm1);
		}
	}
	
	SDL_FreeSurface(surf);
}	
	
void free_heightmap(Heightmap *hm) {
	hm->w = 0;
	hm->h = 0;
	
	free(hm->verts);
	free(hm->normals);
}

void draw_heightmap(Heightmap *hm) {
	int x, y;
	Vertex *v;
	Vector *n;
	
	glPushMatrix();
	glTranslatef(0,100,0);
	glRotatef(55, 1,0,0);
	
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glColor4f(0.5,0.5,0.5,1);
		
	for(y = 0; y < hm->h-1; y++) {
		for(x = 0; x < hm->w-1; x++) {			
			if(hm->verts[y*hm->w+x].y + global.frames*5 > SCREEN_HEIGHT+100 || hm->verts[y*hm->w+x].y + global.frames*5 < -10 ) continue;
									
			glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, 60);
			glBegin(GL_TRIANGLES);
				n = &hm->normals[y*hm->w+x*2];
				glNormal3f(n->x, n->y, n->z);
				
				v = &hm->verts[y*hm->w+x];
				glVertex3f(v->x, v->y + global.frames*5, v->z);
				v = &hm->verts[y*hm->w+x+1];
				glVertex3f(v->x, v->y + global.frames*5, v->z);
				v = &hm->verts[(y+1)*hm->w+x];
				glVertex3f(v->x, v->y + global.frames*5, v->z);

				n = &hm->normals[y*hm->w+x*2+1];
				glNormal3f(n->x, n->y, n->z);

				v = &hm->verts[y*hm->w+x+1];
				glVertex3f(v->x, v->y + global.frames*5, v->z);
				v = &hm->verts[(y+1)*hm->w+x+1];
				glVertex3f(v->x, v->y + global.frames*5, v->z);
				v = &hm->verts[(y+1)*hm->w+x];
				glVertex3f(v->x, v->y + global.frames*5, v->z);
			glEnd();
		}
	}
	glDisable(GL_LIGHTING);
	
	glPopMatrix();
}