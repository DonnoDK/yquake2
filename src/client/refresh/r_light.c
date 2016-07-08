/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * Lightmaps and dynamic lighting
 *
 * =======================================================================
 */

#include "header/local.h"

#define DLIGHT_CUTOFF 64

int r_dlightframecount;
vec3_t pointcolor;
vec3_t lightspot;
static float s_blocklights[34 * 34 * 3];

void R_RenderDlight(dlight_t *light) {
	glBegin(GL_TRIANGLE_FAN);
	glColor3f(light->color[0] * 0.2, light->color[1] * 0.2, light->color[2] * 0.2);

	vec3_t v;
	VectorSubtract(light->origin, r_origin, v);
	float rad = light->intensity * 0.35;
	for (int i = 0; i < 3; i++) {
		v[i] = light->origin[i] - vpn[i] * rad;
	}

	glVertex3fv(v);
	glColor3f(0, 0, 0);

	for (int i = 16; i >= 0; i--) {
		float a = i / 16.0 * M_PI * 2;
		for (int j = 0; j < 3; j++) {
			v[j] = light->origin[j] + vright[j] * cos(a) * rad + vup[j] * sin(a) * rad;
		}
		glVertex3fv(v);
	}
	glEnd();
}

void R_RenderDlights(dlight_t* dlights, int num_dlights) {
	if (!gl_flashblend->value) {
		return;
	}

	/* because the count hasn't advanced yet for this frame */
	r_dlightframecount = r_framecount + 1;

	glDepthMask(0);
	glDisable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	for(int i = 0; i < num_dlights; i++){
        dlight_t* l = &dlights[i];
		R_RenderDlight(l);
	}

	glColor3f(1, 1, 1);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(1);
}

void
R_MarkLights(const dlight_t *light, int bit, mnode_t *node)
{
	cplane_t *splitplane;
	float dist;
	msurface_t *surf;
	int i;
	int sidebit;

	if (node->contents != -1)
	{
		return;
	}

	splitplane = node->plane;
	dist = DotProduct(light->origin, splitplane->normal) - splitplane->dist;

	if (dist > light->intensity - DLIGHT_CUTOFF)
	{
		R_MarkLights(light, bit, node->children[0]);
		return;
	}

	if (dist < -light->intensity + DLIGHT_CUTOFF)
	{
		R_MarkLights(light, bit, node->children[1]);
		return;
	}

	/* mark the polygons */
	surf = r_worldmodel->surfaces + node->firstsurface;

	for (i = 0; i < node->numsurfaces; i++, surf++)
	{
		dist = DotProduct(light->origin, surf->plane->normal) - surf->plane->dist;

		if (dist >= 0)
		{
			sidebit = 0;
		}
		else
		{
			sidebit = SURF_PLANEBACK;
		}

		if ((surf->flags & SURF_PLANEBACK) != sidebit)
		{
			continue;
		}

		if (surf->dlightframe != r_dlightframecount)
		{
			surf->dlightbits = 0;
			surf->dlightframe = r_dlightframecount;
		}

		surf->dlightbits |= bit;
	}

	R_MarkLights(light, bit, node->children[0]);
	R_MarkLights(light, bit, node->children[1]);
}

void R_PushDlights(dlight_t* dlights, int num_dlights) {
	if (gl_flashblend->value) {
		return;
	}
	/* because the count hasn't advanced yet for this frame */
	r_dlightframecount = r_framecount + 1;
	for (int i = 0; i < num_dlights; i++) {
        dlight_t* l = &dlights[i];
		R_MarkLights(l, 1 << i, r_worldmodel->nodes);
	}
}

static int R_RecursiveLightPoint(mnode_t *node, vec3_t start, vec3_t end, lightstyle_t* lightstyles) {
	if (node->contents != -1) {
		return -1;     /* didn't hit anything */
	}

	/* calculate mid point */
	cplane_t* plane = node->plane;
	float front = DotProduct(start, plane->normal) - plane->dist;
	float back = DotProduct(end, plane->normal) - plane->dist;
	int side = front < 0;

	if ((back < 0) == side) {
		return R_RecursiveLightPoint(node->children[side], start, end, lightstyles);
	}

	float frac = front / (front - back);
	vec3_t mid;
	mid[0] = start[0] + (end[0] - start[0]) * frac;
	mid[1] = start[1] + (end[1] - start[1]) * frac;
	mid[2] = start[2] + (end[2] - start[2]) * frac;

	/* go down front side */
	int r = R_RecursiveLightPoint(node->children[side], start, mid, lightstyles);
	if (r >= 0) {
		return r;     /* hit something */
	}

	if ((back < 0) == side) {
		return -1;     /* didn't hit anuthing */
	}

	/* check for impact on this node */
	VectorCopy(mid, lightspot);

	for (int i = 0; i < node->numsurfaces; i++) {
        msurface_t* surf = &r_worldmodel->surfaces[node->firstsurface + i];

		if (surf->flags & (SURF_DRAWTURB | SURF_DRAWSKY)) {
			continue; /* no lightmaps */
		}

		mtexinfo_t* tex = surf->texinfo;

		int s = DotProduct(mid, tex->vecs[0]) + tex->vecs[0][3];
		int t = DotProduct(mid, tex->vecs[1]) + tex->vecs[1][3];

		if ((s < surf->texturemins[0]) || (t < surf->texturemins[1])) {
			continue;
		}

		int ds = s - surf->texturemins[0];
		int dt = t - surf->texturemins[1];

		if ((ds > surf->extents[0]) || (dt > surf->extents[1])) {
			continue;
		}

		if (!surf->samples) {
			return 0;
		}

		ds >>= 4;
		dt >>= 4;

		byte* lightmap = surf->samples;
		VectorCopy(vec3_origin, pointcolor);

		if (lightmap) {
			lightmap += 3 * (dt * ((surf->extents[0] >> 4) + 1) + ds);

			for (int maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++) {
                vec3_t scale;
				for (int i = 0; i < 3; i++) {
					scale[i] = gl_modulate->value * lightstyles[surf->styles[maps]].rgb[i];
				}

				pointcolor[0] += lightmap[0] * scale[0] * (1.0 / 255);
				pointcolor[1] += lightmap[1] * scale[1] * (1.0 / 255);
				pointcolor[2] += lightmap[2] * scale[2] * (1.0 / 255);
				lightmap += 3 * ((surf->extents[0] >> 4) + 1) * ((surf->extents[1] >> 4) + 1);
			}
		}
		return 1;
	}
	/* go down back side */
	return R_RecursiveLightPoint(node->children[!side], mid, end, lightstyles);
}

void R_LightPoint(vec3_t ent_origin, vec3_t p, vec3_t color, dlight_t* dlights, int num_dlights, lightstyle_t* lightstyles) {
	if (!r_worldmodel->lightdata) {
		color[0] = color[1] = color[2] = 1.0;
		return;
	}

	vec3_t end;
	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 2048;

	float r = R_RecursiveLightPoint(r_worldmodel->nodes, p, end, lightstyles);
	if (r == -1) {
		VectorCopy(vec3_origin, color);
	} else {
		VectorCopy(pointcolor, color);
	}

	/* add dynamic lights */
	for (int i = 0; i < num_dlights; i++) {
        dlight_t* dl = &dlights[i];
        vec3_t dist;
		VectorSubtract(ent_origin, dl->origin, dist);
		float add = dl->intensity - VectorLength(dist);
		add *= (1.0 / 256);
		if (add > 0) {
			VectorMA(color, add, dl->color, color);
		}
	}

	VectorScale(color, gl_modulate->value, color);
}

static void R_AddDynamicLights(msurface_t *surf, dlight_t* dlights, int num_dlights) {
	int smax = (surf->extents[0] >> 4) + 1;
	int tmax = (surf->extents[1] >> 4) + 1;
	mtexinfo_t* tex = surf->texinfo;

	for (int i = 0; i < num_dlights; i++) {
		if (!(surf->dlightbits & (1 << i))) {
			continue; /* not lit by this light */
		}

		dlight_t* dl = &dlights[i];
		float frad = dl->intensity;
		float fdist = DotProduct(dl->origin, surf->plane->normal) -
				surf->plane->dist;
		frad -= fabs(fdist);

		/* rad is now the highest intensity on the plane */
		float fminlight = DLIGHT_CUTOFF;

		if (frad < fminlight) {
			continue;
		}

		fminlight = frad - fminlight;

        vec3_t impact;
		for (int i = 0; i < 3; i++) {
			impact[i] = dl->origin[i] - surf->plane->normal[i] * fdist;
		}

        vec3_t local;
		local[0] = DotProduct(impact, tex->vecs[0]) + tex->vecs[0][3] - surf->texturemins[0];
		local[1] = DotProduct(impact, tex->vecs[1]) + tex->vecs[1][3] - surf->texturemins[1];

		float* pfBL = s_blocklights;

        float ftacc;
        int t;
		for (t = 0, ftacc = 0; t < tmax; t++, ftacc += 16) {
			int td = local[1] - ftacc;
			if (td < 0) {
				td = -td;
			}
            float fsacc;
            int s;
			for (s = 0, fsacc = 0; s < smax; s++, fsacc += 16, pfBL += 3) {
				int sd = Q_ftol(local[0] - fsacc);
				if (sd < 0) {
					sd = -sd;
				}
				if (sd > td) {
					fdist = sd + (td >> 1);
				} else {
					fdist = td + (sd >> 1);
				}
				if (fdist < fminlight) {
					pfBL[0] += (frad - fdist) * dl->color[0];
					pfBL[1] += (frad - fdist) * dl->color[1];
					pfBL[2] += (frad - fdist) * dl->color[2];
				}
			}
		}
	}
}

void R_SetCacheState(msurface_t *surf, lightstyle_t* lightstyles) {
	for (int maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++) {
		surf->cached_light[maps] = lightstyles[surf->styles[maps]].white;
	}
}

/*
 * Combine and scale multiple lightmaps into the floating format in blocklights
 */
void R_BuildLightMap(msurface_t *surf, byte *dest, int stride) {
	int smax, tmax;
	int r, g, b, a, max;
	int i, j, size;
	byte *lightmap;
	float scale[4];
	int nummaps;
	float *bl;

    /* TODO: refactor this to params */
    lightstyle_t* lightstyles = r_newrefdef.lightstyles;
    dlight_t* dlights = r_newrefdef.dlights;
    int num_dlights = r_newrefdef.num_dlights;

	if (surf->texinfo->flags &
		(SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP))
	{
		VID_Error(ERR_DROP, "R_BuildLightMap called for non-lit surface");
	}

	smax = (surf->extents[0] >> 4) + 1;
	tmax = (surf->extents[1] >> 4) + 1;
	size = smax * tmax;

	if (size > (sizeof(s_blocklights) >> 4))
	{
		VID_Error(ERR_DROP, "Bad s_blocklights size");
	}

	/* set to full bright if no light data */
	if (!surf->samples)
	{
		for (i = 0; i < size * 3; i++)
		{
			s_blocklights[i] = 255;
		}

		goto store;
	}

	/* count the # of maps */
	for (nummaps = 0; nummaps < MAXLIGHTMAPS && surf->styles[nummaps] != 255;
		 nummaps++)
	{
	}

	lightmap = surf->samples;

	/* add all the lightmaps */
	if (nummaps == 1)
	{
		int maps;

		for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
		{
			bl = s_blocklights;

			for (i = 0; i < 3; i++)
			{
				scale[i] = gl_modulate->value *
						   lightstyles[surf->styles[maps]].rgb[i];
			}

			if ((scale[0] == 1.0F) &&
				(scale[1] == 1.0F) &&
				(scale[2] == 1.0F))
			{
				for (i = 0; i < size; i++, bl += 3)
				{
					bl[0] = lightmap[i * 3 + 0];
					bl[1] = lightmap[i * 3 + 1];
					bl[2] = lightmap[i * 3 + 2];
				}
			}
			else
			{
				for (i = 0; i < size; i++, bl += 3)
				{
					bl[0] = lightmap[i * 3 + 0] * scale[0];
					bl[1] = lightmap[i * 3 + 1] * scale[1];
					bl[2] = lightmap[i * 3 + 2] * scale[2];
				}
			}

			lightmap += size * 3; /* skip to next lightmap */
		}
	}
	else
	{
		int maps;

		memset(s_blocklights, 0, sizeof(s_blocklights[0]) * size * 3);

		for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++)
		{
			bl = s_blocklights;

			for (i = 0; i < 3; i++)
			{
				scale[i] = gl_modulate->value *
						   lightstyles[surf->styles[maps]].rgb[i];
			}

			if ((scale[0] == 1.0F) &&
				(scale[1] == 1.0F) &&
				(scale[2] == 1.0F))
			{
				for (i = 0; i < size; i++, bl += 3)
				{
					bl[0] += lightmap[i * 3 + 0];
					bl[1] += lightmap[i * 3 + 1];
					bl[2] += lightmap[i * 3 + 2];
				}
			}
			else
			{
				for (i = 0; i < size; i++, bl += 3)
				{
					bl[0] += lightmap[i * 3 + 0] * scale[0];
					bl[1] += lightmap[i * 3 + 1] * scale[1];
					bl[2] += lightmap[i * 3 + 2] * scale[2];
				}
			}

			lightmap += size * 3; /* skip to next lightmap */
		}
	}

	/* add all the dynamic lights */
	if (surf->dlightframe == r_framecount) {
		R_AddDynamicLights(surf, dlights, num_dlights);
	}

store:

	stride -= (smax << 2);
	bl = s_blocklights;

	for (i = 0; i < tmax; i++, dest += stride)
	{
		for (j = 0; j < smax; j++)
		{
			r = Q_ftol(bl[0]);
			g = Q_ftol(bl[1]);
			b = Q_ftol(bl[2]);

			/* catch negative lights */
			if (r < 0)
			{
				r = 0;
			}

			if (g < 0)
			{
				g = 0;
			}

			if (b < 0)
			{
				b = 0;
			}

			/* determine the brightest of the three color components */
			if (r > g)
			{
				max = r;
			}
			else
			{
				max = g;
			}

			if (b > max)
			{
				max = b;
			}

			/* alpha is ONLY used for the mono lightmap case. For this
			   reason we set it to the brightest of the color components
			   so that things don't get too dim. */
			a = max;

			/* rescale all the color components if the
			   intensity of the greatest channel exceeds
			   1.0 */
			if (max > 255)
			{
				float t = 255.0F / max;

				r = r * t;
				g = g * t;
				b = b * t;
				a = a * t;
			}

			dest[0] = r;
			dest[1] = g;
			dest[2] = b;
			dest[3] = a;

			bl += 3;
			dest += 4;
		}
	}
}

