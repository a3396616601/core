/* Copyright (C) 2006 - 2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/* ScriptData
SDName: Boss_Murmur
SD%Complete: 90
SDComment: Timers may be incorrect
SDCategory: Auchindoun, Shadow Labyrinth
EndScriptData */

#include "precompiled.h"

#define EMOTE_SONIC_BOOM            -1555036

#define SPELL_SONIC_BOOM_CAST       (HeroicMode?38796:33923)
#define SPELL_SONIC_BOOM_EFFECT     (HeroicMode?38795:33666)
#define SPELL_RESONANCE             33657
#define SPELL_MURMURS_TOUCH         (HeroicMode?38794:33711)
#define SPELL_MAGNETIC_PULL         33689
#define SPELL_SONIC_SHOCK           38797
#define SPELL_THUNDERING_STORM      39365

struct TRINITY_DLL_DECL boss_murmurAI : public Scripted_NoMovementAI
{
    boss_murmurAI(Creature *c) : Scripted_NoMovementAI(c)
    {
        HeroicMode = m_creature->GetMap()->IsHeroic();
    }

    uint32 SonicBoom_Timer;
    uint32 MurmursTouch_Timer;
    uint32 Resonance_Timer;
    uint32 MagneticPull_Timer;
    uint32 SonicShock_Timer;
    uint32 ThunderingStorm_Timer;
    bool HeroicMode;

    void Reset()
    {
        SonicBoom_Timer = 30000;
        MurmursTouch_Timer = 20000;
        Resonance_Timer = 10000;
        MagneticPull_Timer = 20000;
        ThunderingStorm_Timer = 15000;
        SonicShock_Timer = 10000;

        //database should have `RegenHealth`=0 to prevent regen
        uint32 hp = (m_creature->GetMaxHealth()*40)/100;
        if (hp)
            m_creature->SetHealth(hp);

        m_creature->ResetPlayerDamageReq();
    }

    void EnterCombat(Unit *who) { }

    // Sonic Boom instant damage (needs core fix instead of this)
    void SpellHitTarget(Unit *target, const SpellEntry *spell)
    {
        if(target && target->isAlive() && spell && spell->Id == SPELL_SONIC_BOOM_EFFECT)
            m_creature->DealDamage(target, (target->GetHealth()*90)/100, SPELL_DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NATURE, spell);
    }

    void UpdateAI(const uint32 diff)
    {
        //Return since we have no target or casting
        if (!UpdateVictim() || m_creature->IsNonMeleeSpellCasted(false))
            return;

        // Murmur's Touch
        if (MurmursTouch_Timer < diff)
        {
            if(Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0, 80, true))
                AddSpellToCast(target, SPELL_MURMURS_TOUCH);

            MurmursTouch_Timer = 30000;
        }
        else
            MurmursTouch_Timer -= diff;

        // Resonance
        if(Resonance_Timer < diff)
        {
            if(!m_creature->hasUnitState(UNIT_STAT_CASTING))
            {
                Unit *target = SelectUnit(SELECT_TARGET_NEAREST, 0, 100, true);

                if(target && !m_creature->IsWithinMeleeRange(target))
                    AddSpellToCast(m_creature, SPELL_RESONANCE);

                Resonance_Timer = 5000;
            }
            else
                Resonance_Timer = 2000;
        }
        else
            Resonance_Timer -= diff;

        if(HeroicMode)
        {
            // Thundering Storm
            if(ThunderingStorm_Timer < diff)
            {
                ForceSpellCast(SPELL_THUNDERING_STORM, CAST_NULL, DONT_INTERRUPT);

                ThunderingStorm_Timer = 5000;
            }
            else
                ThunderingStorm_Timer -= diff;

            // Sonic Shock
            if(SonicShock_Timer < diff)
            {
                if(Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0, 100, false))
                    AddSpellToCast(target, SPELL_SONIC_SHOCK);

                SonicShock_Timer = urand(10000, 20000);
            }
            else
                SonicShock_Timer -= diff;
        }

        // Magnetic Pull
        if (MagneticPull_Timer < diff)
        {
            if(Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0, 100, true))
            {
                ForceSpellCast(target, SPELL_MAGNETIC_PULL);
                MagneticPull_Timer = urand (20000, 35000);
            }
            else
                MagneticPull_Timer = 500;
        }
        else
            MagneticPull_Timer -= diff;

        // Sonic Boom
        if(SonicBoom_Timer < diff)
        {
            ForceSpellCast(m_creature, SPELL_SONIC_BOOM_EFFECT, DONT_INTERRUPT, true);
            ForceSpellCastWithScriptText(m_creature, SPELL_SONIC_BOOM_CAST, EMOTE_SONIC_BOOM);
            SonicBoom_Timer = 30000;
            Resonance_Timer = 1500;
        }
        else
            SonicBoom_Timer -= diff;

        CastNextSpellIfAnyAndReady();

        // Select nearest most aggro target if top aggro too far
        if(!m_creature->isAttackReady())
            return;

        if(!m_creature->IsWithinMeleeRange(m_creature->getVictim()))
        {
            std::list<HostilReference*>& m_threatlist = m_creature->getThreatManager().getThreatList();
            for(std::list<HostilReference*>::iterator i = m_threatlist.begin(); i != m_threatlist.end(); ++i)
            {
                if(Unit* target = Unit::GetUnit((*m_creature),(*i)->getUnitGuid()))
                {
                    if(target->isAlive() && m_creature->IsWithinMeleeRange(target))
                    {
                        m_creature->TauntApply(target);
                        break;
                    }
                }
            }
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_murmur(Creature *_Creature)
{
    return new boss_murmurAI (_Creature);
}

void AddSC_boss_murmur()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name="boss_murmur";
    newscript->GetAI = &GetAI_boss_murmur;
    newscript->RegisterSelf();
}
