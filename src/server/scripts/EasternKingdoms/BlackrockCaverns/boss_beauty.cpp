#include "ScriptPCH.h"
#include "blackrock_caverns.h"

enum Spells
{
    SPELL_BERSERK                  = 82395,
    SPELL_BERSERKER_CHARGE         = 76030,
    SPELL_FLAMEBREAK               = 76032,
    SPELL_MAGMA_SPLIT              = 76031,
    SPELL_MAGMA_SPLIT_EXPLODE_1 = 76058,
    SPELL_MAGMA_SPLIT_EXPLODE_2 = 76072,
    SPELL_MAGMA_SPLIT_EXPLODE_3 = 76074,
    SPELL_MAGMA_SPLIT_EXPLODE_4 = 76076,
    SPELL_MAGMA_BLAST_EFF = 76070,
    SPELL_TERRIFYING_ROAR          = 76028,
};

enum events
{
    EVENT_MAGMA_SPLIT              = 1,
    EVENT_TERRIFYING_ROAR          = 2,
    EVENT_FLAMEBREAK               = 3,
    EVENT_BERSERKER_CHARGE         = 4,
};

enum Adds
{
    NPC_SPOT                       = 40011,
    NPC_BUSTER                     = 40013,
    NPC_LUCKY                      = 40008,
    NPC_RUNTY                      = 40015,
};

const std::vector<uint32> beautyMinions =
{
    NPC_SPOT,
    NPC_BUSTER,
    NPC_LUCKY,
    NPC_RUNTY,
};

const Position pupsPos[2] = 
{
    { 116.70f, 572.05f, 76.45f, 0.04f },
    { 119.81f, 588.90f, 76.35f, 6.22f }
};

class boss_beauty : public CreatureScript
{
    public:
        boss_beauty() : CreatureScript("boss_beauty") { }
     
        struct boss_beautyAI : public BossAI
        {
            boss_beautyAI(Creature* creature) : BossAI(creature, DATA_BEAUTY)
            {
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
                me->setActive(true);
            }

            void Reset() override
            {
                _Reset();
                summons.DespawnAll();

                if (instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

                // Respawn Minions
                for (auto&& itr : beautyMinions)
                    if (Creature* myDog = me->FindNearestCreature(itr, 100.0f, false))
                        myDog->Respawn();
            }

            void EnterCombat(Unit* /*who*/) override
            {
                _EnterCombat();

                if (instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);

                events.ScheduleEvent(EVENT_BERSERKER_CHARGE, 1000);
                events.ScheduleEvent(EVENT_FLAMEBREAK, 15000);
                events.ScheduleEvent(EVENT_MAGMA_SPLIT, 18000);
                events.ScheduleEvent(EVENT_TERRIFYING_ROAR, 30000);
                DoZoneInCombat();
            }

            void JustDied(Unit* /*killer*/) override
            {
                _JustDied();
                summons.DespawnAll();

                if (instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }

            void JustSummoned(Creature* summon) override
            {
                summons.Summon(summon);
            }

            void SummonedCreatureDespawn(Creature* summon) override
            {
                summons.Despawn(summon);
            }
     
            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                if (me->GetDistance(me->GetHomePosition()) > 60.0f)
                {
                    EnterEvadeMode();
                    return;
                }

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
                
                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_BERSERKER_CHARGE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1))
                                DoCast(target, SPELL_BERSERKER_CHARGE);
                            events.ScheduleEvent(EVENT_BERSERKER_CHARGE, 10000);
                            break;
                        case EVENT_FLAMEBREAK:
                            DoCast(me, SPELL_FLAMEBREAK);
                            events.ScheduleEvent(EVENT_FLAMEBREAK, 15000);
                            break;
                        case EVENT_MAGMA_SPLIT:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                                DoCast(target, SPELL_MAGMA_SPLIT);
                            events.ScheduleEvent(EVENT_MAGMA_SPLIT, 18000);
                            break;
                        case EVENT_TERRIFYING_ROAR:
                            DoCast(SPELL_TERRIFYING_ROAR);
                            events.ScheduleEvent(EVENT_TERRIFYING_ROAR, 30000);
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

            private:
                //Creature* pups[2];
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<boss_beautyAI>(creature);
        }
};
 
// Runty 40015
struct npc_runty : public SmartCreatureAI
{
    npc_runty(Creature* creature) : SmartCreatureAI(creature) { }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        DoMeleeAttackIfReady();
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (Creature* beauty = ObjectAccessor::GetCreature(*me, me->GetInstanceScript() ? me->GetInstanceScript()->GetData64(DATA_BEAUTY) : 0))
            beauty->CastSpell(beauty, SPELL_BERSERK, true);
    }
};

// Magma Split 76031
class spell_beauty_magma_split : public AuraScript
{
    PrepareAuraScript(spell_beauty_magma_split);

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Unit* owner = GetOwner()->ToUnit())
        {
            owner->CastSpell(owner, SPELL_MAGMA_SPLIT_EXPLODE_1, true);
            owner->CastSpell(owner, SPELL_MAGMA_SPLIT_EXPLODE_2, true);
            owner->CastSpell(owner, SPELL_MAGMA_SPLIT_EXPLODE_3, true);
            owner->CastSpell(owner, SPELL_MAGMA_SPLIT_EXPLODE_4, true);
        }
    }

    void Register() override
    {
        OnEffectRemove += AuraEffectRemoveFn(spell_beauty_magma_split::OnRemove, EFFECT_1, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
    }
};

void AddSC_boss_beauty()
{
    new boss_beauty();
    new creature_script<npc_runty>("npc_runty");
    new aura_script<spell_beauty_magma_split>("spell_beauty_magma_split");
}
