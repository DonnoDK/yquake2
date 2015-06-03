/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2011 Yamagi Burmeister
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
 * Game fields to be saved.
 *
 * =======================================================================
 */

{"classname", (int)FOFS(classname), F_LSTRING},
{"model", (int)FOFS(model), F_LSTRING},
{"spawnflags", (int)FOFS(spawnflags), F_INT},
{"speed", (int)FOFS(speed), F_FLOAT},
{"accel", (int)FOFS(accel), F_FLOAT},
{"decel", (int)FOFS(decel), F_FLOAT},
{"target", (int)FOFS(target), F_LSTRING},
{"targetname", (int)FOFS(targetname), F_LSTRING},
{"pathtarget", (int)FOFS(pathtarget), F_LSTRING},
{"deathtarget", (int)FOFS(deathtarget), F_LSTRING},
{"killtarget", (int)FOFS(killtarget), F_LSTRING},
{"combattarget", (int)FOFS(combattarget), F_LSTRING},
{"message", (int)FOFS(message), F_LSTRING},
{"team", (int)FOFS(team), F_LSTRING},
{"wait", (int)FOFS(wait), F_FLOAT},
{"delay", (int)FOFS(delay), F_FLOAT},
{"random", (int)FOFS(random), F_FLOAT},
{"move_origin", (int)FOFS(move_origin), F_VECTOR},
{"move_angles", (int)FOFS(move_angles), F_VECTOR},
{"style", (int)FOFS(style), F_INT},
{"count", (int)FOFS(count), F_INT},
{"health", (int)FOFS(health), F_INT},
{"sounds", (int)FOFS(sounds), F_INT},
{"light", 0, F_IGNORE},
{"dmg", (int)FOFS(dmg), F_INT},
{"mass", (int)FOFS(mass), F_INT},
{"volume", (int)FOFS(volume), F_FLOAT},
{"attenuation", (int)FOFS(attenuation), F_FLOAT},
{"map", (int)FOFS(map), F_LSTRING},
{"origin", (int)FOFS(s.origin), F_VECTOR},
{"angles", (int)FOFS(s.angles), F_VECTOR},
{"angle", (int)FOFS(s.angles), F_ANGLEHACK},
{"goalentity", (int)FOFS(goalentity), F_EDICT, FFL_NOSPAWN},
{"movetarget", (int)FOFS(movetarget), F_EDICT, FFL_NOSPAWN},
{"enemy", (int)FOFS(enemy), F_EDICT, FFL_NOSPAWN},
{"oldenemy", (int)FOFS(oldenemy), F_EDICT, FFL_NOSPAWN},
{"activator", (int)FOFS(activator), F_EDICT, FFL_NOSPAWN},
{"groundentity", (int)FOFS(groundentity), F_EDICT, FFL_NOSPAWN},
{"teamchain", (int)FOFS(teamchain), F_EDICT, FFL_NOSPAWN},
{"teammaster", (int)FOFS(teammaster), F_EDICT, FFL_NOSPAWN},
{"owner", (int)FOFS(owner), F_EDICT, FFL_NOSPAWN},
{"mynoise", (int)FOFS(mynoise), F_EDICT, FFL_NOSPAWN},
{"mynoise2", (int)FOFS(mynoise2), F_EDICT, FFL_NOSPAWN},
{"target_ent", (int)FOFS(target_ent), F_EDICT, FFL_NOSPAWN},
{"chain", (int)FOFS(chain), F_EDICT, FFL_NOSPAWN},
{"prethink", (int)FOFS(prethink), F_FUNCTION, FFL_NOSPAWN},
{"think", (int)FOFS(think), F_FUNCTION, FFL_NOSPAWN},
{"blocked", (int)FOFS(blocked), F_FUNCTION, FFL_NOSPAWN},
{"touch", (int)FOFS(touch), F_FUNCTION, FFL_NOSPAWN},
{"use", (int)FOFS(use), F_FUNCTION, FFL_NOSPAWN},
{"pain", (int)FOFS(pain), F_FUNCTION, FFL_NOSPAWN},
{"die", (int)FOFS(die), F_FUNCTION, FFL_NOSPAWN},
{"stand", (int)FOFS(monsterinfo.stand), F_FUNCTION, FFL_NOSPAWN},
{"idle", (int)FOFS(monsterinfo.idle), F_FUNCTION, FFL_NOSPAWN},
{"search", (int)FOFS(monsterinfo.search), F_FUNCTION, FFL_NOSPAWN},
{"walk", (int)FOFS(monsterinfo.walk), F_FUNCTION, FFL_NOSPAWN},
{"run", (int)FOFS(monsterinfo.run), F_FUNCTION, FFL_NOSPAWN},
{"dodge", (int)FOFS(monsterinfo.dodge), F_FUNCTION, FFL_NOSPAWN},
{"attack", (int)FOFS(monsterinfo.attack), F_FUNCTION, FFL_NOSPAWN},
{"melee", (int)FOFS(monsterinfo.melee), F_FUNCTION, FFL_NOSPAWN},
{"sight", (int)FOFS(monsterinfo.sight), F_FUNCTION, FFL_NOSPAWN},
{"checkattack", (int)FOFS(monsterinfo.checkattack), F_FUNCTION, FFL_NOSPAWN},
{"currentmove", (int)FOFS(monsterinfo.currentmove), F_MMOVE, FFL_NOSPAWN},
{"endfunc", (int)FOFS(moveinfo.endfunc), F_FUNCTION, FFL_NOSPAWN},
{"lip", (int)STOFS(lip), F_INT, FFL_SPAWNTEMP},
{"distance", (int)STOFS(distance), F_INT, FFL_SPAWNTEMP},
{"height", (int)STOFS(height), F_INT, FFL_SPAWNTEMP},
{"noise", (int)STOFS(noise), F_LSTRING, FFL_SPAWNTEMP},
{"pausetime", (int)STOFS(pausetime), F_FLOAT, FFL_SPAWNTEMP},
{"item", (int)STOFS(item), F_LSTRING, FFL_SPAWNTEMP},
{"item", (int)FOFS(item), F_ITEM},
{"gravity", (int)STOFS(gravity), F_LSTRING, FFL_SPAWNTEMP},
{"sky", (int)STOFS(sky), F_LSTRING, FFL_SPAWNTEMP},
{"skyrotate", (int)STOFS(skyrotate), F_FLOAT, FFL_SPAWNTEMP},
{"skyaxis", (int)STOFS(skyaxis), F_VECTOR, FFL_SPAWNTEMP},
{"minyaw", (int)STOFS(minyaw), F_FLOAT, FFL_SPAWNTEMP},
{"maxyaw", (int)STOFS(maxyaw), F_FLOAT, FFL_SPAWNTEMP},
{"minpitch", (int)STOFS(minpitch), F_FLOAT, FFL_SPAWNTEMP},
{"maxpitch", (int)STOFS(maxpitch), F_FLOAT, FFL_SPAWNTEMP},
{"nextmap", (int)STOFS(nextmap), F_LSTRING, FFL_SPAWNTEMP},
{0, 0, (fieldtype_t)0, 0}
