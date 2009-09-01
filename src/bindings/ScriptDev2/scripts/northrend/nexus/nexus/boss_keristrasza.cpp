/* Copyright (C) 2006 - 2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
SDName: Boss_Keristrasza
SD%Complete: 
SDComment: 
SDCategory: The Nexus, The Nexus
EndScriptData */

#include "precompiled.h"
#include "def_nexus.h"

//Spells
#define SPELL_FROZEN_PRISON                             47854
#define SPELL_TAIL_SWEEP                                50155
#define SPELL_CRYSTAL_CHAINS                            50997
#define SPELL_ENRAGE                                    8599
#define SPELL_CRYSTALFIRE_BREATH_N                      48096
#define SPELL_CRYSTALFIRE_BREATH_H                      57091
#define SPELL_CRYSTALIZE                                48179
#define SPELL_INTENSE_COLD                              48094
#define SPELL_INTENSE_COLD_TRIGGERED                    48095

enum
{
    CONTAINMENT_SPHERES         = 3,

    SAY_AGGRO                   = -1576016,
    SAY_CRYSTAL_NOVA            = -1576017,
    SAY_ENRAGE                  = -1576018,
    SAY_KILL                    = -1576019,
    SAY_DEATH                   = -1576020,
};

struct MANGOS_DLL_DECL boss_keristraszaAI : public ScriptedAI
{
    boss_keristraszaAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        pInstance = ((ScriptedInstance*)pCreature->GetInstanceData());
        HeroicMode = pCreature->GetMap()->IsHeroic();
        Reset();
    }

    ScriptedInstance* pInstance;
    bool HeroicMode;

    uint32 CRYSTALFIRE_BREATH_Timer;
    uint32 CRYSTAL_CHAINS_CRYSTALIZE_Timer;
    uint32 TAIL_SWEEP_Timer;
    uint32 StartCombat_Timer;
    bool StartCombat;
    bool Enrage;
    uint64 ContainmentSphereGUIDs[CONTAINMENT_SPHERES];

    void Reset()
    {
        CRYSTALFIRE_BREATH_Timer = 14000;
        CRYSTAL_CHAINS_CRYSTALIZE_Timer = HeroicMode ? 30000 : 11000;
        TAIL_SWEEP_Timer = 5000;
        Enrage = false;
        StartCombat_Timer = 2000;
        StartCombat = false;
        memset(&ContainmentSphereGUIDs, 0, sizeof(ContainmentSphereGUIDs));

        m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);

        RemovePrison(CheckContainmentSpheres());

        if (pInstance)
            pInstance->SetData(DATA_KERISTRASZA_EVENT, NOT_STARTED);
    }

    void Aggro(Unit* pWho)
    {
        DoScriptText(SAY_AGGRO, m_creature);
        DoCast(m_creature, SPELL_INTENSE_COLD);

        if (pInstance)
            pInstance->SetData(DATA_KERISTRASZA_EVENT, IN_PROGRESS);
    }

    void JustDied(Unit* pKiller)  
    {
        DoScriptText(SAY_DEATH, m_creature);

        if (pInstance)
            pInstance->SetData(DATA_KERISTRASZA_EVENT, DONE);
    }

    void KilledUnit(Unit* pVictim)
    {
        DoScriptText(SAY_KILL, m_creature);
    }

    bool CheckContainmentSpheres(bool remove_prison = false)
    {
        ContainmentSphereGUIDs[0] = pInstance->GetData64(ANOMALUS_CONTAINMET_SPHERE);
        ContainmentSphereGUIDs[1] = pInstance->GetData64(ORMOROKS_CONTAINMET_SPHERE);
        ContainmentSphereGUIDs[2] = pInstance->GetData64(TELESTRAS_CONTAINMET_SPHERE);

        GameObject* ContainmentSpheres[CONTAINMENT_SPHERES];

        for (uint8 i = 0; i < CONTAINMENT_SPHERES; i++)
        {
            ContainmentSpheres[i] = pInstance->instance->GetGameObject(ContainmentSphereGUIDs[i]);
            if (!ContainmentSpheres[i])
                return false;
            if (ContainmentSpheres[i]->GetGoState() != GO_STATE_ACTIVE)
                return false;
        }
        
        if (remove_prison)
            RemovePrison(true);
        
        return true;
    }

    void RemovePrison(bool remove)
    {
        if (remove)
        {
            m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            if (m_creature->HasAura(SPELL_FROZEN_PRISON))
            {
                StartCombat = true;
                m_creature->RemoveAurasDueToSpell(SPELL_FROZEN_PRISON);
            }
        }
        else
        {
            m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            m_creature->CastSpell(m_creature, SPELL_FROZEN_PRISON, false);
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if (StartCombat)
            if (StartCombat_Timer < diff)
            {
                StartCombat = false;
                m_creature->SetInCombatWithZone();
            }else StartCombat_Timer -=diff;
        
        if (!m_creature->SelectHostilTarget() || !m_creature->getVictim())
            return;
          
        if (!Enrage && (m_creature->GetHealth() < m_creature->GetMaxHealth() * 0.25))
        {
            DoScriptText(SAY_ENRAGE , m_creature);
            DoCast(m_creature, SPELL_ENRAGE);
            Enrage = true;
        }

        if (CRYSTALFIRE_BREATH_Timer < diff)
        {
            DoCast(m_creature->getVictim(), HeroicMode ? SPELL_CRYSTALFIRE_BREATH_H : SPELL_CRYSTALFIRE_BREATH_N);
            CRYSTALFIRE_BREATH_Timer = 14000;
        }else CRYSTALFIRE_BREATH_Timer -=diff;

        if (TAIL_SWEEP_Timer < diff)
        {
            DoCast(m_creature, SPELL_TAIL_SWEEP);
            TAIL_SWEEP_Timer = 5000;
        }else TAIL_SWEEP_Timer -=diff;

        if (CRYSTAL_CHAINS_CRYSTALIZE_Timer < diff)
        {
            DoScriptText(SAY_CRYSTAL_NOVA , m_creature);
            if (HeroicMode)
                DoCast(m_creature, SPELL_CRYSTALIZE);
            else
                if (Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0))
                    DoCast(target, SPELL_CRYSTAL_CHAINS);
            CRYSTAL_CHAINS_CRYSTALIZE_Timer = HeroicMode ? 30000 : 11000;
        }else CRYSTAL_CHAINS_CRYSTALIZE_Timer -= diff;

        DoMeleeAttackIfReady();
    }
};

bool GOHello_containment_sphere(Player* pPlayer, GameObject* pGO)
{
    ScriptedInstance* pInstance = (ScriptedInstance*)pGO->GetInstanceData();

    Unit* Keristrasza = Unit::GetUnit(*pGO, pInstance->GetData64(DATA_KERISTRASZA));
    if (Keristrasza && Keristrasza->isAlive())
    {
        // maybe these are hacks :(
        pGO->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_UNK1);
        pGO->SetGoState(GO_STATE_ACTIVE);

        ((boss_keristraszaAI*)((Creature*)Keristrasza)->AI())->CheckContainmentSpheres(true);
    }
    return true;
}

CreatureAI* GetAI_boss_keristrasza(Creature* pCreature)
{
    return new boss_keristraszaAI (pCreature);
}

void AddSC_boss_keristrasza()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "boss_keristrasza";
    newscript->GetAI = &GetAI_boss_keristrasza;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "containment_sphere";
    newscript->pGOHello = &GOHello_containment_sphere;
    newscript->RegisterSelf();
}
