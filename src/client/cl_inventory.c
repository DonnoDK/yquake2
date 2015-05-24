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
 * This file implements the inventory screen
 *
 * =======================================================================
 */

#include "header/client.h"

void CL_ParseInventory(sizebuf_t* message){
    for(int i = 0; i < MAX_ITEMS; i++){
        cl.inventory[i] = MSG_ReadShort(message);
    }
}

static void SetStringHighBit(char *s){
    while(*s){
        *s++ |= 128;
    }
}

#define DISPLAY_ITEMS 17

void CL_DrawInventory(void){
    int index[MAX_ITEMS];
    char string[1024];
    char binding[1024];
    int selected = cl.frame.playerstate.stats[STAT_SELECTED_ITEM];
    int num = 0;
    int selected_num = 0;
    float scale = SCR_GetHUDScale();
    for(int i = 0; i < MAX_ITEMS; i++){
        if(i == selected){
            selected_num = num;
        }
        if(cl.inventory[i]){
            index[num] = i;
            num++;
        }
    }
    /* determine scroll point */
    int top = selected_num - DISPLAY_ITEMS / 2;
    if(num - top < DISPLAY_ITEMS){
        top = num - DISPLAY_ITEMS;
    }
    if(top < 0){
        top = 0;
    }
    int x = (viddef.width - scale*256) / 2;
    int y = (viddef.height - scale*240) / 2;
    /* repaint everything next frame */
    SCR_DirtyScreen();
    Draw_PicScaled(x, y + scale*8, "inventory", scale);
    y += scale*24;
    x += scale*24;
    DrawStringScaled(x, y, "hotkey ### item", scale);
    DrawStringScaled(x, y + scale*8, "------ --- ----", scale);
    y += scale*16;
    for(int i = top; i < num && i < top + DISPLAY_ITEMS; i++){
        int item = index[i];
        /* search for a binding */
        Com_sprintf(binding, sizeof(binding), "use %s", cl.configstrings[CS_ITEMS + item]);
        const char* bind = "";
        for(int j = 0; j < 256; j++){
            if(keybindings[j] && !Q_stricmp(keybindings[j], binding)){
                bind = Key_KeynumToString(j);
                break;
            }
        }
        Com_sprintf(string, sizeof(string), "%6s %3i %s", bind, cl.inventory[item], cl.configstrings[CS_ITEMS + item]);
        if(item != selected){
            SetStringHighBit(string);
        }else{
            /* draw a blinky cursor by the selected item */
            if((int)(cls.realtime * 10) & 1){
                Draw_CharScaled(x - scale*8, y, 15, scale);
            }
        }
        DrawStringScaled(x, y, string, scale);
        y += scale*8;
    }
}
