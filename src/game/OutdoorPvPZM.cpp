/*
 * Copyright (C) 2005-2008 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "OutdoorPvPZM.h"
#include "ObjectMgr.h"
#include "OutdoorPvPMgr.h"
#include "Player.h"
#include "Creature.h"
#include "ObjectAccessor.h"
#include "WorldPacket.h"
#include "GossipDef.h"
#include "World.h"

OutdoorPvPZM::OutdoorPvPZM()
{
    m_GraveYard = NULL;
    m_AllianceTowersControlled = 0;
    m_HordeTowersControlled = 0;

}

OutdoorPvPObjectiveZM_Beacon::OutdoorPvPObjectiveZM_Beacon(OutdoorPvP *pvp, ZM_BeaconType type)
: OutdoorPvPObjective(pvp), m_TowerType(type), m_TowerState(ZM_TOWERSTATE_N)
{
    AddCapturePoint(ZMCapturePoints[type].entry,ZMCapturePoints[type].map,ZMCapturePoints[type].x,ZMCapturePoints[type].y,ZMCapturePoints[type].z,ZMCapturePoints[type].o,ZMCapturePoints[type].rot0,ZMCapturePoints[type].rot1,ZMCapturePoints[type].rot2,ZMCapturePoints[type].rot3);
}

void OutdoorPvPObjectiveZM_Beacon::FillInitialWorldStates(WorldPacket &data)
{
    data << uint32(ZMBeaconInfo[m_TowerType].ui_tower_n) << uint32(bool(m_TowerState & ZM_TOWERSTATE_N));
    data << uint32(ZMBeaconInfo[m_TowerType].map_tower_n) << uint32(bool(m_TowerState & ZM_TOWERSTATE_N));
    data << uint32(ZMBeaconInfo[m_TowerType].ui_tower_a) << uint32(bool(m_TowerState & ZM_TOWERSTATE_A));
    data << uint32(ZMBeaconInfo[m_TowerType].map_tower_a) << uint32(bool(m_TowerState & ZM_TOWERSTATE_A));
    data << uint32(ZMBeaconInfo[m_TowerType].ui_tower_h) << uint32(bool(m_TowerState & ZM_TOWERSTATE_H));
    data << uint32(ZMBeaconInfo[m_TowerType].map_tower_h) << uint32(bool(m_TowerState & ZM_TOWERSTATE_H));
}

void OutdoorPvPObjectiveZM_Beacon::UpdateTowerState()
{
    m_PvP->SendUpdateWorldState(uint32(ZMBeaconInfo[m_TowerType].ui_tower_n),uint32(bool(m_TowerState & ZM_TOWERSTATE_N)));
    m_PvP->SendUpdateWorldState(uint32(ZMBeaconInfo[m_TowerType].map_tower_n),uint32(bool(m_TowerState & ZM_TOWERSTATE_N)));
    m_PvP->SendUpdateWorldState(uint32(ZMBeaconInfo[m_TowerType].ui_tower_a),uint32(bool(m_TowerState & ZM_TOWERSTATE_A)));
    m_PvP->SendUpdateWorldState(uint32(ZMBeaconInfo[m_TowerType].map_tower_a),uint32(bool(m_TowerState & ZM_TOWERSTATE_A)));
    m_PvP->SendUpdateWorldState(uint32(ZMBeaconInfo[m_TowerType].ui_tower_h),uint32(bool(m_TowerState & ZM_TOWERSTATE_H)));
    m_PvP->SendUpdateWorldState(uint32(ZMBeaconInfo[m_TowerType].map_tower_h),uint32(bool(m_TowerState & ZM_TOWERSTATE_H)));
}

void OutdoorPvPObjectiveZM_Beacon::HandlePlayerEnter(Player *plr)
{
    OutdoorPvPObjective::HandlePlayerEnter(plr);
    plr->SendUpdateWorldState(ZMBeaconInfo[m_TowerType].slider_disp, 1);
    uint32 phase = (uint32)ceil(( m_ShiftPhase + m_ShiftMaxPhase) / ( 2 * m_ShiftMaxPhase ) * 100.0f);
    plr->SendUpdateWorldState(ZMBeaconInfo[m_TowerType].slider_pos, phase);
    plr->SendUpdateWorldState(ZMBeaconInfo[m_TowerType].slider_n, m_NeutralValue);
}

void OutdoorPvPObjectiveZM_Beacon::HandlePlayerLeave(Player *plr)
{
    plr->SendUpdateWorldState(ZMBeaconInfo[m_TowerType].slider_disp, 0);
    OutdoorPvPObjective::HandlePlayerLeave(plr);
}

bool OutdoorPvPObjectiveZM_Beacon::HandleCapturePointEvent(Player *plr, uint32 eventId)
{
    if(eventId == ZMBeaconInfo[m_TowerType].event_enter)
    {
        this->HandlePlayerEnter(plr);
        return true;
    }
    else if (eventId == ZMBeaconInfo[m_TowerType].event_leave)
    {
        this->HandlePlayerLeave(plr);
        return true;
    }
    return false;
}

bool OutdoorPvPObjectiveZM_Beacon::Update(uint32 diff)
{
    if(OutdoorPvPObjective::Update(diff))
    {
        if(m_OldState != m_State)
        {
            // if changing from controlling alliance to horde
            if( m_OldState == OBJECTIVESTATE_ALLIANCE )
            {
                if(((OutdoorPvPZM*)m_PvP)->m_AllianceTowersControlled)
                    ((OutdoorPvPZM*)m_PvP)->m_AllianceTowersControlled--;
                sWorld.SendZoneText(ZM_GRAVEYARD_ZONE,objmgr.GetMangosString(ZMBeaconLooseA[m_TowerType], -1));
            }
            // if changing from controlling horde to alliance
            else if ( m_OldState == OBJECTIVESTATE_HORDE )
            {
                if(((OutdoorPvPZM*)m_PvP)->m_HordeTowersControlled)
                    ((OutdoorPvPZM*)m_PvP)->m_HordeTowersControlled--;
                sWorld.SendZoneText(ZM_GRAVEYARD_ZONE,objmgr.GetMangosString(ZMBeaconLooseH[m_TowerType],-1));
            }

            switch(m_State)
            {
                case OBJECTIVESTATE_ALLIANCE:
                    m_TowerState = ZM_TOWERSTATE_A;
                    if(((OutdoorPvPZM*)m_PvP)->m_AllianceTowersControlled<ZM_NUM_BEACONS)
                        ((OutdoorPvPZM*)m_PvP)->m_AllianceTowersControlled++;
                    sWorld.SendZoneText(ZM_GRAVEYARD_ZONE,objmgr.GetMangosString(ZMBeaconCaptureA[m_TowerType],-1));
                    break;
                case OBJECTIVESTATE_HORDE:
                    m_TowerState = ZM_TOWERSTATE_H;
                    if(((OutdoorPvPZM*)m_PvP)->m_HordeTowersControlled<ZM_NUM_BEACONS)
                        ((OutdoorPvPZM*)m_PvP)->m_HordeTowersControlled++;
                    sWorld.SendZoneText(ZM_GRAVEYARD_ZONE,objmgr.GetMangosString(ZMBeaconCaptureH[m_TowerType],-1));
                    break;
                case OBJECTIVESTATE_NEUTRAL:
                case OBJECTIVESTATE_NEUTRAL_ALLIANCE_CHALLENGE:
                case OBJECTIVESTATE_NEUTRAL_HORDE_CHALLENGE:
                case OBJECTIVESTATE_ALLIANCE_HORDE_CHALLENGE:
                case OBJECTIVESTATE_HORDE_ALLIANCE_CHALLENGE:
                    m_TowerState = ZM_TOWERSTATE_N;
                    break;
            }

            UpdateTowerState();
        }

        if(m_ShiftPhase != m_OldPhase)
        {
            // send this too, sometimes the slider disappears, dunno why :(
            SendUpdateWorldState(ZMBeaconInfo[m_TowerType].slider_disp, 1);
            // send these updates to only the ones in this objective
            uint32 phase = (uint32)ceil(( m_ShiftPhase + m_ShiftMaxPhase) / ( 2 * m_ShiftMaxPhase ) * 100.0f);
            SendUpdateWorldState(ZMBeaconInfo[m_TowerType].slider_pos, phase);
            SendUpdateWorldState(ZMBeaconInfo[m_TowerType].slider_n, m_NeutralValue);
        }
        return true;
    }
    return false;
}

bool OutdoorPvPZM::Update(uint32 diff)
{
    bool changed = false;
    if(changed = OutdoorPvP::Update(diff))
    {
        if(m_AllianceTowersControlled == ZM_NUM_BEACONS)
            m_GraveYard->SetBeaconState(ALLIANCE);
        else if(m_HordeTowersControlled == ZM_NUM_BEACONS)
            m_GraveYard->SetBeaconState(HORDE);
        else
            m_GraveYard->SetBeaconState(0);
    }
    return changed;
}

void OutdoorPvPZM::HandlePlayerEnterZone(Player * plr, uint32 zone)
{
    if(plr->GetTeam() == ALLIANCE)
    {
        if(m_GraveYard->m_GraveYardState & ZM_GRAVEYARD_A)
            plr->CastSpell(plr,ZM_CAPTURE_BUFF,true);
    }
    else
    {
        if(m_GraveYard->m_GraveYardState & ZM_GRAVEYARD_H)
            plr->CastSpell(plr,ZM_CAPTURE_BUFF,true);
    }
    OutdoorPvP::HandlePlayerEnterZone(plr,zone);
}

void OutdoorPvPZM::HandlePlayerLeaveZone(Player * plr, uint32 zone)
{
    // remove buffs
    plr->RemoveAurasDueToSpell(ZM_CAPTURE_BUFF);
    // remove flag
    plr->RemoveAurasDueToSpell(ZM_BATTLE_STANDARD_A);
    plr->RemoveAurasDueToSpell(ZM_BATTLE_STANDARD_H);
    OutdoorPvP::HandlePlayerLeaveZone(plr, zone);
}

bool OutdoorPvPZM::SetupOutdoorPvP()
{
    m_AllianceTowersControlled = 0;
    m_HordeTowersControlled = 0;

    // add the zones affected by the pvp buff
    for(int i = 0; i < OutdoorPvPZMBuffZonesNum; ++i)
        sOutdoorPvPMgr.AddZone(OutdoorPvPZMBuffZones[i],this);

    m_OutdoorPvPObjectives.insert(new OutdoorPvPObjectiveZM_Beacon(this,ZM_BEACON_WEST));
    m_OutdoorPvPObjectives.insert(new OutdoorPvPObjectiveZM_Beacon(this,ZM_BEACON_EAST));
    m_GraveYard = new OutdoorPvPObjectiveZM_GraveYard(this);
    m_OutdoorPvPObjectives.insert(m_GraveYard); // though the update function isn't used, the handleusego is!

    return true;
}

void OutdoorPvPZM::HandleKillImpl(Player *plr, Unit * killed)
{
    if(killed->GetTypeId() != TYPEID_PLAYER)
        return;

    if(plr->GetTeam() == ALLIANCE && ((Player*)killed)->GetTeam() != ALLIANCE)
        plr->CastSpell(plr,ZM_AlliancePlayerKillReward,true);
    else if(plr->GetTeam() == HORDE && ((Player*)killed)->GetTeam() != HORDE)
        plr->CastSpell(plr,ZM_HordePlayerKillReward,true);
}

void OutdoorPvPZM::BuffTeam(uint32 team)
{
    if(team == ALLIANCE)
    {
        for(std::set<uint64>::iterator itr = m_PlayerGuids[0].begin(); itr != m_PlayerGuids[0].end(); ++itr)
        {
            if(Player * plr = objmgr.GetPlayer(*itr))
                if(plr->IsInWorld()) plr->CastSpell(plr,ZM_CAPTURE_BUFF,true);
        }
        for(std::set<uint64>::iterator itr = m_PlayerGuids[1].begin(); itr != m_PlayerGuids[1].end(); ++itr)
        {
            if(Player * plr = objmgr.GetPlayer(*itr))
                if(plr->IsInWorld()) plr->RemoveAurasDueToSpell(ZM_CAPTURE_BUFF);
        }
    }
    else if(team == HORDE)
    {
        for(std::set<uint64>::iterator itr = m_PlayerGuids[1].begin(); itr != m_PlayerGuids[1].end(); ++itr)
        {
            if(Player * plr = objmgr.GetPlayer(*itr))
                if(plr->IsInWorld()) plr->CastSpell(plr,ZM_CAPTURE_BUFF,true);
        }
        for(std::set<uint64>::iterator itr = m_PlayerGuids[0].begin(); itr != m_PlayerGuids[0].end(); ++itr)
        {
            if(Player * plr = objmgr.GetPlayer(*itr))
                if(plr->IsInWorld()) plr->RemoveAurasDueToSpell(ZM_CAPTURE_BUFF);
        }
    }
    else
    {
        for(std::set<uint64>::iterator itr = m_PlayerGuids[0].begin(); itr != m_PlayerGuids[0].end(); ++itr)
        {
            if(Player * plr = objmgr.GetPlayer(*itr))
                if(plr->IsInWorld()) plr->RemoveAurasDueToSpell(ZM_CAPTURE_BUFF);
        }
        for(std::set<uint64>::iterator itr = m_PlayerGuids[1].begin(); itr != m_PlayerGuids[1].end(); ++itr)
        {
            if(Player * plr = objmgr.GetPlayer(*itr))
                if(plr->IsInWorld()) plr->RemoveAurasDueToSpell(ZM_CAPTURE_BUFF);
        }
    }
}

bool OutdoorPvPObjectiveZM_GraveYard::Update(uint32 diff)
{
    bool retval = m_State != m_OldState;
    m_State = m_OldState;
    return retval;
}

int32 OutdoorPvPObjectiveZM_GraveYard::HandleOpenGo(Player *plr, uint64 guid)
{
    uint32 retval = OutdoorPvPObjective::HandleOpenGo(plr, guid);
    if(retval>=0)
    {
        if(plr->HasAura(ZM_BATTLE_STANDARD_A,0) && m_GraveYardState != ZM_GRAVEYARD_A)
        {
            if(m_GraveYardState == ZM_GRAVEYARD_H)
                sWorld.SendZoneText(ZM_GRAVEYARD_ZONE,objmgr.GetMangosString(LANG_OPVP_ZM_LOOSE_GY_H,-1));
            m_GraveYardState = ZM_GRAVEYARD_A;
            DelObject(0);   // only one gotype is used in the whole outdoor pvp, no need to call it a constant
            AddObject(0,ZM_Banner_A.entry,ZM_Banner_A.map,ZM_Banner_A.x,ZM_Banner_A.y,ZM_Banner_A.z,ZM_Banner_A.o,ZM_Banner_A.rot0,ZM_Banner_A.rot1,ZM_Banner_A.rot2,ZM_Banner_A.rot3);
            objmgr.RemoveGraveYardLink(ZM_GRAVEYARD_ID, ZM_GRAVEYARD_ZONE, HORDE);          // rem gy
            objmgr.AddGraveYardLink(ZM_GRAVEYARD_ID, ZM_GRAVEYARD_ZONE, ALLIANCE, false);   // add gy
            ((OutdoorPvPZM*)m_PvP)->BuffTeam(ALLIANCE);
            plr->RemoveAurasDueToSpell(ZM_BATTLE_STANDARD_A);
            sWorld.SendZoneText(ZM_GRAVEYARD_ZONE,objmgr.GetMangosString(LANG_OPVP_ZM_CAPTURE_GY_A, -1));
        }
        else if(plr->HasAura(ZM_BATTLE_STANDARD_H,0) && m_GraveYardState != ZM_GRAVEYARD_H)
        {
            if(m_GraveYardState == ZM_GRAVEYARD_A)
                sWorld.SendZoneText(ZM_GRAVEYARD_ZONE,objmgr.GetMangosString(LANG_OPVP_ZM_LOOSE_GY_A,-1));
            m_GraveYardState = ZM_GRAVEYARD_H;
            DelObject(0);   // only one gotype is used in the whole outdoor pvp, no need to call it a constant
            AddObject(0,ZM_Banner_H.entry,ZM_Banner_H.map,ZM_Banner_H.x,ZM_Banner_H.y,ZM_Banner_H.z,ZM_Banner_H.o,ZM_Banner_H.rot0,ZM_Banner_H.rot1,ZM_Banner_H.rot2,ZM_Banner_H.rot3);
            objmgr.RemoveGraveYardLink(ZM_GRAVEYARD_ID, ZM_GRAVEYARD_ZONE, ALLIANCE);          // rem gy
            objmgr.AddGraveYardLink(ZM_GRAVEYARD_ID, ZM_GRAVEYARD_ZONE, HORDE, false);   // add gy
            ((OutdoorPvPZM*)m_PvP)->BuffTeam(HORDE);
            plr->RemoveAurasDueToSpell(ZM_BATTLE_STANDARD_H);
            sWorld.SendZoneText(ZM_GRAVEYARD_ZONE,objmgr.GetMangosString(LANG_OPVP_ZM_CAPTURE_GY_H,-1));
        }
        UpdateTowerState();
    }
    return retval;
}

OutdoorPvPObjectiveZM_GraveYard::OutdoorPvPObjectiveZM_GraveYard(OutdoorPvP *pvp)
: OutdoorPvPObjective(pvp)
{
    m_BothControllingFaction = 0;
    m_GraveYardState = ZM_GRAVEYARD_N;
    m_FlagCarrierGUID = 0;
    // add field scouts here
    AddCreature(ZM_ALLIANCE_FIELD_SCOUT,ZM_AllianceFieldScout.entry,ZM_AllianceFieldScout.teamval,ZM_AllianceFieldScout.map,ZM_AllianceFieldScout.x,ZM_AllianceFieldScout.y,ZM_AllianceFieldScout.z,ZM_AllianceFieldScout.o);
    AddCreature(ZM_HORDE_FIELD_SCOUT,ZM_HordeFieldScout.entry,ZM_HordeFieldScout.teamval,ZM_HordeFieldScout.map,ZM_HordeFieldScout.x,ZM_HordeFieldScout.y,ZM_HordeFieldScout.z,ZM_HordeFieldScout.o);
    // add neutral banner
    AddObject(0,ZM_Banner_N.entry,ZM_Banner_N.map,ZM_Banner_N.x,ZM_Banner_N.y,ZM_Banner_N.z,ZM_Banner_N.o,ZM_Banner_N.rot0,ZM_Banner_N.rot1,ZM_Banner_N.rot2,ZM_Banner_N.rot3);
}

void OutdoorPvPObjectiveZM_GraveYard::UpdateTowerState()
{
    m_PvP->SendUpdateWorldState(ZM_MAP_GRAVEYARD_N,uint32(bool(m_GraveYardState & ZM_GRAVEYARD_N)));
    m_PvP->SendUpdateWorldState(ZM_MAP_GRAVEYARD_H,uint32(bool(m_GraveYardState & ZM_GRAVEYARD_H)));
    m_PvP->SendUpdateWorldState(ZM_MAP_GRAVEYARD_A,uint32(bool(m_GraveYardState & ZM_GRAVEYARD_A)));

    m_PvP->SendUpdateWorldState(ZM_MAP_ALLIANCE_FLAG_READY,uint32(m_BothControllingFaction == ALLIANCE));
    m_PvP->SendUpdateWorldState(ZM_MAP_ALLIANCE_FLAG_NOT_READY,uint32(m_BothControllingFaction != ALLIANCE));
    m_PvP->SendUpdateWorldState(ZM_MAP_HORDE_FLAG_READY,uint32(m_BothControllingFaction == HORDE));
    m_PvP->SendUpdateWorldState(ZM_MAP_HORDE_FLAG_NOT_READY,uint32(m_BothControllingFaction != HORDE));
}

void OutdoorPvPObjectiveZM_GraveYard::FillInitialWorldStates(WorldPacket &data)
{
    data << ZM_MAP_GRAVEYARD_N  << uint32(bool(m_GraveYardState & ZM_GRAVEYARD_N));
    data << ZM_MAP_GRAVEYARD_H  << uint32(bool(m_GraveYardState & ZM_GRAVEYARD_H));
    data << ZM_MAP_GRAVEYARD_A  << uint32(bool(m_GraveYardState & ZM_GRAVEYARD_A));

    data << ZM_MAP_ALLIANCE_FLAG_READY  << uint32(m_BothControllingFaction == ALLIANCE);
    data << ZM_MAP_ALLIANCE_FLAG_NOT_READY  << uint32(m_BothControllingFaction != ALLIANCE);
    data << ZM_MAP_HORDE_FLAG_READY  << uint32(m_BothControllingFaction == HORDE);
    data << ZM_MAP_HORDE_FLAG_NOT_READY  << uint32(m_BothControllingFaction != HORDE);
}

void OutdoorPvPObjectiveZM_GraveYard::SetBeaconState(uint32 controlling_faction)
{
    // nothing to do here
    if(m_BothControllingFaction == controlling_faction)
        return;
    m_BothControllingFaction = controlling_faction;

    switch(controlling_faction)
    {
        case ALLIANCE:
            // if ally already controls the gy and taken back both beacons, return, nothing to do for us
            if(m_GraveYardState & ZM_GRAVEYARD_A)
                return;
            // ally doesn't control the gy, but controls the side beacons -> add gossip option, add neutral banner
            break;
        case HORDE:
            // if horde already controls the gy and taken back both beacons, return, nothing to do for us
            if(m_GraveYardState & ZM_GRAVEYARD_H)
                return;
            // horde doesn't control the gy, but controls the side beacons -> add gossip option, add neutral banner
            break;
        default:
            // if the graveyard is not neutral, then leave it that way
            // if the graveyard is neutral, then we have to dispel the buff from the flag carrier
            if(m_GraveYardState & ZM_GRAVEYARD_N)
            {
                // gy was neutral, thus neutral banner was spawned, it is possible that someone was taking the flag to the gy
                if(m_FlagCarrierGUID)
                {
                    // remove flag from carrier, reset flag carrier guid
                    Player * p = objmgr.GetPlayer(m_FlagCarrierGUID);
                    if(p)
                    {
                        p->RemoveAurasDueToSpell(ZM_BATTLE_STANDARD_A);
                        p->RemoveAurasDueToSpell(ZM_BATTLE_STANDARD_H);
                    }
                    m_FlagCarrierGUID = 0;
                }
            }
            break;
    }
    // send worldstateupdate
    UpdateTowerState();
}

bool OutdoorPvPObjectiveZM_GraveYard::CanTalkTo(Player * plr, Creature * c, GossipOption & gso)
{
    uint64 guid = c->GetGUID();
    std::map<uint64,uint32>::iterator itr = m_CreatureTypes.find(guid);
    if(itr != m_CreatureTypes.end())
    {
        if(itr->second == ZM_ALLIANCE_FIELD_SCOUT && plr->GetTeam() == ALLIANCE && m_BothControllingFaction == ALLIANCE && !m_FlagCarrierGUID && m_GraveYardState != ZM_GRAVEYARD_A)
        {
            gso.OptionText.assign("Give me the flag, I'll take it to the Central Tower for the glory of the Alliance!");
            return true;
        }
        else if(itr->second == ZM_HORDE_FIELD_SCOUT && plr->GetTeam() == HORDE && m_BothControllingFaction == HORDE && !m_FlagCarrierGUID && m_GraveYardState != ZM_GRAVEYARD_H)
        {
            gso.OptionText.assign("Give me the flag, I'll take it to the Central Tower for the glory of the Horde!");
            return true;
        }
    }
    return false;
}

bool OutdoorPvPObjectiveZM_GraveYard::HandleGossipOption(Player *plr, uint64 guid, uint32 gossipid)
{
    std::map<uint64,uint32>::iterator itr = m_CreatureTypes.find(guid);
    if(itr != m_CreatureTypes.end())
    {
        Creature * cr = HashMapHolder<Creature>::Find(guid);
        if(!cr)
            return true;
        // if the flag is already taken, then return
        if(m_FlagCarrierGUID)
            return true;
        if(itr->second == ZM_ALLIANCE_FIELD_SCOUT)
        {
            cr->CastSpell(plr,ZM_BATTLE_STANDARD_A,true);
            m_FlagCarrierGUID = plr->GetGUID();
        }
        else if(itr->second == ZM_HORDE_FIELD_SCOUT)
        {
            cr->CastSpell(plr,ZM_BATTLE_STANDARD_H,true);
            m_FlagCarrierGUID = plr->GetGUID();
        }
        UpdateTowerState();
        plr->PlayerTalkClass->CloseGossip();
        return true;
    }
    return false;
}

bool OutdoorPvPObjectiveZM_GraveYard::HandleDropFlag(Player * plr, uint32 spellId)
{
    switch(spellId)
    {
        case ZM_BATTLE_STANDARD_A:
            m_FlagCarrierGUID = 0;
            return true;
        case ZM_BATTLE_STANDARD_H:
            m_FlagCarrierGUID = 0;
            return true;
    }
    return false;
}

void OutdoorPvPZM::FillInitialWorldStates(WorldPacket &data)
{
    data << ZM_WORLDSTATE_UNK_1 << uint32(1);
    for(OutdoorPvPObjectiveSet::iterator itr = m_OutdoorPvPObjectives.begin(); itr != m_OutdoorPvPObjectives.end(); ++itr)
    {
        (*itr)->FillInitialWorldStates(data);
    }
}

void OutdoorPvPZM::SendRemoveWorldStates(Player *plr)
{
    plr->SendUpdateWorldState(ZM_UI_TOWER_SLIDER_N_W,0);
    plr->SendUpdateWorldState(ZM_UI_TOWER_SLIDER_POS_W,0);
    plr->SendUpdateWorldState(ZM_UI_TOWER_SLIDER_DISPLAY_W,0);
    plr->SendUpdateWorldState(ZM_UI_TOWER_SLIDER_N_E,0);
    plr->SendUpdateWorldState(ZM_UI_TOWER_SLIDER_POS_E,0);
    plr->SendUpdateWorldState(ZM_UI_TOWER_SLIDER_DISPLAY_E,0);
    plr->SendUpdateWorldState(ZM_WORLDSTATE_UNK_1,1);
    plr->SendUpdateWorldState(ZM_UI_TOWER_EAST_N,0);
    plr->SendUpdateWorldState(ZM_UI_TOWER_EAST_H,0);
    plr->SendUpdateWorldState(ZM_UI_TOWER_EAST_A,0);
    plr->SendUpdateWorldState(ZM_UI_TOWER_WEST_N,0);
    plr->SendUpdateWorldState(ZM_UI_TOWER_WEST_H,0);
    plr->SendUpdateWorldState(ZM_UI_TOWER_WEST_A,0);
    plr->SendUpdateWorldState(ZM_MAP_TOWER_EAST_N,0);
    plr->SendUpdateWorldState(ZM_MAP_TOWER_EAST_H,0);
    plr->SendUpdateWorldState(ZM_MAP_TOWER_EAST_A,0);
    plr->SendUpdateWorldState(ZM_MAP_GRAVEYARD_H,0);
    plr->SendUpdateWorldState(ZM_MAP_GRAVEYARD_A,0);
    plr->SendUpdateWorldState(ZM_MAP_GRAVEYARD_N,0);
    plr->SendUpdateWorldState(ZM_MAP_TOWER_WEST_N,0);
    plr->SendUpdateWorldState(ZM_MAP_TOWER_WEST_H,0);
    plr->SendUpdateWorldState(ZM_MAP_TOWER_WEST_A,0);
    plr->SendUpdateWorldState(ZM_MAP_HORDE_FLAG_READY,0);
    plr->SendUpdateWorldState(ZM_MAP_HORDE_FLAG_NOT_READY,0);
    plr->SendUpdateWorldState(ZM_MAP_ALLIANCE_FLAG_NOT_READY,0);
    plr->SendUpdateWorldState(ZM_MAP_ALLIANCE_FLAG_READY,0);
}
