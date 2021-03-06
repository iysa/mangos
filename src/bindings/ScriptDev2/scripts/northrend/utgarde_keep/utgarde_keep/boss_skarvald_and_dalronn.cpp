/* ScriptData
SDName: Boss_Skarvald_Dalronn
SD%Complete: 95
SDComment: Needs adjustments to blizzlike timers, Yell Text + Sound to DB
SDCategory: Utgarde Keep
EndScriptData */

#include "precompiled.h"
#include "sc_creature.h"
#include "def_utgarde_keep.h"

#define SAY_SKARVALD_AGGRO                          -1574011
#define SAY_DALRONN_AGGRO                           -1574016

#define SAY_SKARVALD_KILL                           -1574014
#define SAY_DALRONN_KILL                            -1574019

#define SAY_DALRONN_DAL_DIEDFIRST                   -1574017
#define SAY_SKARVALD_DAL_DIEDFIRST                  -1574015
#define SAY_SKARVALD_DAL_DIED                       -1574013

#define SAY_SKARVALD_SKA_DIEDFIRST                  -1574012
#define SAY_DALRONN_SKA_DIEDFIRST                   -1574020
#define SAY_DALRONN_SKA_DIED                        -1574018

//Spells of Skarvald and his Ghost
#define MOB_SKARVALD_THE_CONSTRUCTOR                24200
#define SPELL_CHARGE                                43651
#define SPELL_STONE_STRIKE                          48583
#define SPELL_SUMMON_SKARVALD_GHOST                 48613
#define MOB_SKARVALD_GHOST                          27390
//Spells of Dalronn and his Ghost
#define MOB_DALRONN_THE_CONTROLLER                  24201
#define SPELL_SHADOW_BOLT                           43649
#define H_SPELL_SHADOW_BOLT                         59575
#define H_SPELL_SUMMON_SKELETONS                    52611
#define SPELL_DEBILITATE                            43650
#define SPELL_SUMMON_DALRONN_GHOST                  48612
#define MOB_DALRONN_GHOST                           27389

struct MANGOS_DLL_DECL boss_skarvald_the_constructorAI : public ScriptedAI
{
    boss_skarvald_the_constructorAI(Creature *c) : ScriptedAI(c) 
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        HeroicMode = m_creature->GetMap()->IsHeroic();
        Reset();
    }

    ScriptedInstance* pInstance;

    bool ghost;
    bool HeroicMode;
    uint32 Charge_Timer;
    uint32 StoneStrike_Timer;
    uint32 Response_Timer;
    uint32 Check_Timer;
    bool Dalronn_isDead;

    void Reset()
    {
        Charge_Timer = 5000;
        StoneStrike_Timer = 10000;
        Dalronn_isDead = false;
        Check_Timer = 5000;

		if (m_creature->GetEntry() == MOB_SKARVALD_GHOST)
			ghost = true;
		else
			ghost = false;

        if(ghost)
        {
            m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        } else {
            if (pInstance)
			{
	            m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
		        m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
				pInstance->SetData(EVENT_SKARVALD_AND_DALRONN, NOT_STARTED);
				Unit* dalronn = Unit::GetUnit((*m_creature),pInstance->GetData64(NPC_DALRONN));
				if(dalronn && dalronn->isDead() && m_creature->isAlive())
					((Creature*)dalronn)->Respawn();
			}
		}
    }

    void Aggro(Unit *who)
    {
        if(!ghost)
        {
            DoScriptText(SAY_SKARVALD_AGGRO,m_creature);

            if (pInstance)
			{
				pInstance->SetData(EVENT_SKARVALD_AND_DALRONN,IN_PROGRESS);
				Unit* dalronn = Unit::GetUnit((*m_creature),pInstance->GetData64(NPC_DALRONN));
				if(dalronn && dalronn->isAlive() && !dalronn->getVictim())
					dalronn->getThreatManager().addThreat(who,0.0f);
			}
        }
    }

    void JustDied(Unit* Killer)
    {
        if(!ghost)
        {
            if (pInstance)
			{
				Unit* dalronn = Unit::GetUnit((*m_creature),pInstance->GetData64(NPC_DALRONN));
				if(dalronn)
				{
					if(dalronn->isDead())
					{
						DoScriptText(SAY_SKARVALD_DAL_DIED, m_creature);

						pInstance->SetData(EVENT_SKARVALD_AND_DALRONN, DONE);
						m_creature->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
						dalronn->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
					}
					else
					{
						DoScriptText(SAY_SKARVALD_SKA_DIEDFIRST, m_creature);
						m_creature->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
						Creature* temp = m_creature->SummonCreature(MOB_SKARVALD_GHOST, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), 0, TEMPSUMMON_CORPSE_DESPAWN,5000);
						temp->AI()->AttackStart(Killer);
					}
				}
			}
        }
    }

    void KilledUnit(Unit *victim)
    {
        if(!ghost)
        {
            DoScriptText(SAY_SKARVALD_KILL,m_creature);
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if(ghost)
        {
            if (pInstance)
				if(pInstance->GetData(EVENT_SKARVALD_AND_DALRONN) != IN_PROGRESS)
				{
					m_creature->DealDamage(m_creature, m_creature->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
				}
        }

       if(!m_creature->SelectHostilTarget() || !m_creature->getVictim() )
            return;

        if(!ghost)
        {
            if(Check_Timer)
                if(Check_Timer < diff)
                {
                    Check_Timer = 5000;
                    if (pInstance)
					{
						Unit* dalronn = Unit::GetUnit((*m_creature),pInstance->GetData64(NPC_DALRONN));
						if(dalronn && dalronn->isDead())
						{
							Dalronn_isDead = true;
							Response_Timer = 2000;
							Check_Timer = 0;
						}
					}
                }else Check_Timer -= diff;

            if(Response_Timer) 
                if(Dalronn_isDead)
                    if(Response_Timer < diff)
                    {
                        DoScriptText(SAY_SKARVALD_DAL_DIEDFIRST,m_creature);

                        Response_Timer = 0;
                    }else Response_Timer -= diff;
        }

        if(Charge_Timer < diff)
        {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 1), SPELL_CHARGE);
            Charge_Timer = 5000+rand()%5000;
        }else Charge_Timer -= diff;

        if(StoneStrike_Timer < diff)
        {
            DoCast(m_creature->getVictim(), SPELL_STONE_STRIKE);
            StoneStrike_Timer = 5000+rand()%5000;
        }else StoneStrike_Timer -= diff;
      
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_skarvald_the_constructor(Creature *_Creature)
{
    return new boss_skarvald_the_constructorAI (_Creature);
}

struct MANGOS_DLL_DECL boss_dalronn_the_controllerAI : public ScriptedAI
{
    boss_dalronn_the_controllerAI(Creature *c) : ScriptedAI(c) 
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        HeroicMode = m_creature->GetMap()->IsHeroic();
        Reset();
    }

    ScriptedInstance* pInstance;

    bool ghost;
    bool HeroicMode;
    uint32 ShadowBolt_Timer;
    uint32 Debilitate_Timer;
    uint32 Summon_Timer;

    uint32 Response_Timer;
    uint32 Check_Timer;
    uint32 AggroYell_Timer;
	uint32 Skeletons_Appear_Timer;
    bool Skarvald_isDead;

    void Reset()
    {
        ShadowBolt_Timer = 1000;
        Debilitate_Timer = 5000;
        Summon_Timer = 10000;
        Check_Timer = 5000;
        Skarvald_isDead = false;
        AggroYell_Timer = 0;
		Skeletons_Appear_Timer = 9999999;

		if (m_creature->GetEntry() == MOB_DALRONN_GHOST)
			ghost = true;
		else
			ghost = false;

		if(ghost)
        {
            m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        } else {
            if (pInstance)
			{
	            m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
		        m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
			    pInstance->SetData(EVENT_SKARVALD_AND_DALRONN, NOT_STARTED);
				Unit* skarvald = Unit::GetUnit((*m_creature), pInstance->GetData64(NPC_SKARVALD));
				if(skarvald && skarvald->isDead() && m_creature->isAlive())
					((Creature*)skarvald)->Respawn();
			}
        }
    }
    void Aggro(Unit *who)
    {
        if(!ghost)
        {
            if (pInstance)
			{
				pInstance->SetData(EVENT_SKARVALD_AND_DALRONN, IN_PROGRESS);
				Unit* skarvald = Unit::GetUnit((*m_creature),pInstance->GetData64(NPC_SKARVALD));
				if(skarvald && skarvald->isAlive() && !skarvald->getVictim())
					skarvald->getThreatManager().addThreat(who,0.0f);
			}

            AggroYell_Timer = 5000;
        }
    }

    void JustDied(Unit* Killer)
    {
        if(!ghost)
        {
            if (pInstance)
			{
				Unit* skarvald = Unit::GetUnit((*m_creature), pInstance->GetData64(NPC_SKARVALD));
				if (skarvald)
					if (skarvald->isDead())
					{
						DoScriptText(SAY_DALRONN_SKA_DIED, m_creature);

						pInstance->SetData(EVENT_SKARVALD_AND_DALRONN, DONE);
						m_creature->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
						skarvald->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
					}
					else
					{
						DoScriptText(SAY_DALRONN_DAL_DIEDFIRST, m_creature);
						m_creature->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
						Creature* temp = m_creature->SummonCreature(MOB_DALRONN_GHOST, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), 0, TEMPSUMMON_CORPSE_DESPAWN,5000);
						temp->AI()->AttackStart(Killer);
					}
			}
        }
    }

    void KilledUnit(Unit *victim)
    {
        if(!ghost)
        {
            DoScriptText(SAY_DALRONN_KILL,m_creature);
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if(ghost)
        {
			if (pInstance)
				if(pInstance->GetData(EVENT_SKARVALD_AND_DALRONN) != IN_PROGRESS)
				{
					m_creature->DealDamage(m_creature, m_creature->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);  
				}
        }

        if(!m_creature->SelectHostilTarget() || !m_creature->getVictim())
            return;

        if(AggroYell_Timer)
            if(AggroYell_Timer < diff)
            {
                DoScriptText(SAY_DALRONN_AGGRO,m_creature);

                AggroYell_Timer = 0;
            }else AggroYell_Timer -= diff;

        if(!ghost)
        {
            if(Check_Timer)
                if(Check_Timer < diff)
                {
                    Check_Timer = 5000;
                    if (pInstance)
					{
						Unit* skarvald = Unit::GetUnit((*m_creature),pInstance->GetData64(NPC_SKARVALD));
						if(skarvald && skarvald->isDead())
						{
							Skarvald_isDead = true;
							Response_Timer = 2000;
							Check_Timer = 0;
						}
					}
                }else Check_Timer -= diff;

            if(Response_Timer)
                if(Skarvald_isDead)
                    if(Response_Timer < diff)
                    {
                        DoScriptText(SAY_DALRONN_SKA_DIEDFIRST,m_creature);

                        Response_Timer = 0;
                    }else Response_Timer -= diff;
        }

        if(ShadowBolt_Timer < diff)
        {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0),HeroicMode ? H_SPELL_SHADOW_BOLT : SPELL_SHADOW_BOLT);
            ShadowBolt_Timer = 2000;
        }else ShadowBolt_Timer -= diff;

        if(Debilitate_Timer < diff)
        {
            m_creature->CastStop();
			DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0),SPELL_DEBILITATE);
            Debilitate_Timer = 7000;
        }else Debilitate_Timer -= diff;
      
        if(HeroicMode)
            if(Summon_Timer < diff)
            {
                m_creature->CastStop();
				DoCast(m_creature, H_SPELL_SUMMON_SKELETONS);
                Summon_Timer = 15000;
				Skeletons_Appear_Timer = 2000;
            }else Summon_Timer -= diff;

		if(Skeletons_Appear_Timer < diff)
		{
			for (int8 i = 1; i<=2; i++)
			{
				Creature* Skeleton = m_creature->SummonCreature(28878, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), m_creature->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 1000);
				Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0);
                if (Skeleton && target)
					Skeleton->AI()->AttackStart(target);
            }
			Skeletons_Appear_Timer = 9999999;
        }else Skeletons_Appear_Timer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_dalronn_the_controller(Creature *_Creature)
{
    return new boss_dalronn_the_controllerAI (_Creature);
}

void AddSC_boss_skarvald_and_dalronn()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "boss_skarvald";
    newscript->GetAI = &GetAI_boss_skarvald_the_constructor;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "boss_dalronn";
    newscript->GetAI = &GetAI_boss_dalronn_the_controller;
    newscript->RegisterSelf();
}
