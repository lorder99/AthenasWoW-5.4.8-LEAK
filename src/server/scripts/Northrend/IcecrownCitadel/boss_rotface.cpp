/*
 * Copyright (C) 2011-2016 Project SkyFire <http://www.projectskyfire.org/>
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2016 MaNGOS <http://getmangos.com/>
 * Copyright (C) 2006-2014 ScriptDev2 <https://github.com/scriptdev2/scriptdev2/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellAuras.h"
#include "GridNotifiers.h"
#include "icecrown_citadel.h"

enum Texts
{
    SAY_PRECIOUS_DIES           = 0,
    SAY_AGGRO                   = 1,
    EMOTE_SLIME_SPRAY           = 2,
    SAY_SLIME_SPRAY             = 3,
    EMOTE_UNSTABLE_EXPLOSION    = 4,
    SAY_UNSTABLE_EXPLOSION      = 5,
    SAY_KILL                    = 6,
    SAY_BERSERK                 = 7,
    SAY_DEATH                   = 8,
    EMOTE_MUTATED_INFECTION     = 9,

    EMOTE_PRECIOUS_ZOMBIES      = 0,
};

enum Spells
{
    // Rotface
    SPELL_SLIME_SPRAY                       = 69508,    // every 20 seconds
    SPELL_VILE_GAS_TRIGGER_SUMMON           = 72287,

    // Oozes
    SPELL_LITTLE_OOZE_COMBINE               = 69537,    // combine 2 Small Oozes
    SPELL_LARGE_OOZE_COMBINE                = 69552,    // combine 2 Large Oozes
    SPELL_LARGE_OOZE_BUFF_COMBINE           = 69611,    // combine Large and Small Ooze
    SPELL_OOZE_MERGE                        = 69889,    // 2 Small Oozes summon a Large Ooze
    SPELL_WEAK_RADIATING_OOZE               = 69750,    // passive damage aura - small
    SPELL_RADIATING_OOZE                    = 69760,    // passive damage aura - large
    SPELL_UNSTABLE_OOZE                     = 69558,    // damage boost and counter for explosion
    SPELL_GREEN_ABOMINATION_HITTIN__YA_PROC = 70001,    // prevents getting hit by infection
    SPELL_UNSTABLE_OOZE_EXPLOSION           = 69839,
    SPELL_STICKY_OOZE                       = 69774,
    SPELL_UNSTABLE_OOZE_EXPLOSION_TRIGGER   = 69832,
    SPELL_VERTEX_COLOR_BRIGHT_RED           = 69844,

    // Precious
    SPELL_MORTAL_WOUND                      = 71127,
    SPELL_DECIMATE                          = 71123,
    SPELL_AWAKEN_PLAGUED_ZOMBIES            = 71159,

    // Professor Putricide
    SPELL_VILE_GAS                          = 72272,
    SPELL_VILE_GAS_TRIGGER                  = 72285,

    SPELL_MUTATED_INFECTION                 = 69674,
};

enum Creatures
{
    NPC_LITTLE_OOZE = 36897,
    NPC_BIG_OOZE    = 36899,
};

enum Events
{
    // Rotface
    EVENT_SLIME_SPRAY = 1,
    EVENT_HASTEN_INFECTIONS,
    EVENT_MUTATED_INFECTION,
    EVENT_VILE_GAS,

    // Precious
    EVENT_DECIMATE,
    EVENT_SUMMON_ZOMBIES,

    EVENT_STICKY_OOZE,
};

class boss_rotface : public CreatureScript
{
    public:
        boss_rotface() : CreatureScript("boss_rotface") { }

        struct boss_rotfaceAI : public BossAI
        {
            boss_rotfaceAI(Creature* creature) : BossAI(creature, DATA_ROTFACE)
            {
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_RESISTANCE, true);
        me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TARGET_RESISTANCE, true);
                infectionStage = 0;
                infectionCooldown = 14000;
            }

            void Reset() override
            {
                _Reset();
                events.ScheduleEvent(EVENT_SLIME_SPRAY, 20000);
                events.ScheduleEvent(EVENT_HASTEN_INFECTIONS, 90000);
                events.ScheduleEvent(EVENT_MUTATED_INFECTION, 14000);
                if (IsHeroic())
                    events.ScheduleEvent(EVENT_VILE_GAS, urand(22000, 27000));

                infectionStage = 0;
                infectionCooldown = 14000;
                aimOrientation = 0.0f;
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
                me->GetMap()->SetWorldState(WORLDSTATE_DANCES_WITH_OOZES, 1);
            }

            void EnterCombat(Unit* who) override
            {
                if (!instance->CheckRequiredBosses(DATA_ROTFACE, who->ToPlayer()))
                {
                    EnterEvadeMode();
                    instance->DoCastSpellOnPlayers(LIGHT_S_HAMMER_TELEPORT);
                    return;
                }

                me->setActive(true);
                Talk(SAY_AGGRO);
                if (Creature* professor = Unit::GetCreature(*me, instance->GetData64(DATA_PROFESSOR_PUTRICIDE)))
                 if (professor->IsAlive())
                    professor->AI()->DoAction(ACTION_ROTFACE_COMBAT);

                DoZoneInCombat();
                DoCast(me, SPELL_GREEN_ABOMINATION_HITTIN__YA_PROC, true);
            }

            void JustDied(Unit* /*killer*/) override
            {
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_MUTATED_INFECTION);
                _JustDied();
                me->RemoveAurasDueToSpell(1130);
                Talk(SAY_DEATH);
                if (Creature* professor = Unit::GetCreature(*me, instance->GetData64(DATA_PROFESSOR_PUTRICIDE)))
              if (professor->IsAlive())
                    professor->AI()->DoAction(ACTION_ROTFACE_DEATH);

                std::list<Creature*> summons;
                GetCreatureListWithEntryInGrid(summons, me, NPC_LITTLE_OOZE, 100.0f);
                GetCreatureListWithEntryInGrid(summons, me, NPC_BIG_OOZE, 100.0f);
                for (std::list<Creature*>::const_iterator itr = summons.begin(); itr != summons.end(); ++itr)
                    if (Creature* summon = *itr)
                        summon->DespawnOrUnsummon(1000);
            }

            void JustReachedHome() override
            {
                _JustReachedHome();
                instance->SetBossState(DATA_ROTFACE, FAIL);
            }

            void KilledUnit(Unit* victim) override
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_KILL);
            }

            void EnterEvadeMode() override
            {
                ScriptedAI::EnterEvadeMode();
                if (Creature* professor = Unit::GetCreature(*me, instance->GetData64(DATA_PROFESSOR_PUTRICIDE)))
                    professor->AI()->EnterEvadeMode();
            }

            void SpellHitTarget(Unit* target, SpellInfo const* spell) override
            {
                if (spell->Id == SPELL_SLIME_SPRAY)
                {
                    Talk(SAY_SLIME_SPRAY);
                    aimOrientation = me->GetAngle(target);
                }
            }

            void JustSummoned(Creature* summon) override
            {
                if (summon->GetEntry() == NPC_VILE_GAS_STALKER)
                    if (Creature* professor = Unit::GetCreature(*me, instance->GetData64(DATA_PROFESSOR_PUTRICIDE)))
                        professor->CastSpell(summon, SPELL_VILE_GAS, true);
            }

            void SummonedCreatureDespawn(Creature* summon) override
            {
                if (summon)
                {
                    if (summon->GetEntry() == NPC_OOZE_SPRAY_STALKER)
                    {
                        aimOrientation = 0.0f;
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
                    }
                }
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim() || !CheckInRoom())
                    return;

                events.Update(diff);

                if (aimOrientation != 0.0f)
                {
                    me->SetFacingTo(aimOrientation);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
                    return;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SLIME_SPRAY:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                            {
                                if (Creature* orientationTarget = DoSummon(NPC_OOZE_SPRAY_STALKER, *target, 8000, TEMPSUMMON_TIMED_DESPAWN))
                                {
                                    Talk(EMOTE_SLIME_SPRAY);
                                    DoCast(me, SPELL_SLIME_SPRAY);
                                }
                            }
                            events.ScheduleEvent(EVENT_SLIME_SPRAY, 20000);
                            break;
                        case EVENT_HASTEN_INFECTIONS:
                            if (infectionStage++ < 4)
                            {
                                infectionCooldown -= 2000;
                                events.ScheduleEvent(EVENT_HASTEN_INFECTIONS, 90000);
                            }
                            break;
                        case EVENT_MUTATED_INFECTION:
                        {
                            Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true, -SPELL_MUTATED_INFECTION);
                            if (!target)
                                target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true, -SPELL_MUTATED_INFECTION);
                            if (target)
                                me->AddAura(SPELL_MUTATED_INFECTION, target);
                            events.ScheduleEvent(EVENT_MUTATED_INFECTION, infectionCooldown);
                            break;
                        }
                        case EVENT_VILE_GAS:
                            DoCastAOE(SPELL_VILE_GAS_TRIGGER);
                            events.ScheduleEvent(EVENT_VILE_GAS, urand(30000, 35000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            uint32 infectionCooldown;
            uint32 infectionStage;
            float aimOrientation;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetIcecrownCitadelAI<boss_rotfaceAI>(creature);
        }
};

class npc_little_ooze : public CreatureScript
{
    public:
        npc_little_ooze() : CreatureScript("npc_little_ooze") { }

        struct npc_little_oozeAI : public ScriptedAI
        {
            npc_little_oozeAI(Creature* creature) : ScriptedAI(creature), instance(creature->GetInstanceScript())
            {
            }

            void IsSummonedBy(Unit* summoner)
            {
                DoCast(me, SPELL_LITTLE_OOZE_COMBINE, true);
                DoCast(me, SPELL_WEAK_RADIATING_OOZE, true);
                DoCast(me, SPELL_GREEN_ABOMINATION_HITTIN__YA_PROC, true);
                events.ScheduleEvent(EVENT_STICKY_OOZE, 5000);
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
                me->AddThreat(summoner, 500000.0f);
            }

            void JustDied(Unit* /*killer*/) override
            {
                me->DespawnOrUnsummon();
            }

            void UpdateAI(uint32 diff) override
            {
                if (InstanceScript* instance = me->GetInstanceScript())
                {
                    if (instance->GetBossState(DATA_ROTFACE) != IN_PROGRESS)
                    {
                        me->DespawnOrUnsummon();
                        return;
                    }
                }

                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (events.ExecuteEvent() == EVENT_STICKY_OOZE)
                {
                    //DoCastVictim(SPELL_STICKY_OOZE);
                    // Set Rotface to original caster for this, no effect of sticky ooze will be triggered if ooze gets despawned
                    me->CastSpell(me->GetVictim(), SPELL_STICKY_OOZE, false, 0, 0, instance->GetData64(DATA_ROTFACE));
                    events.ScheduleEvent(EVENT_STICKY_OOZE, 15000);
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap events;
            InstanceScript* instance;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetIcecrownCitadelAI<npc_little_oozeAI>(creature);
        }
};

class npc_big_ooze : public CreatureScript
{
    public:
        npc_big_ooze() : CreatureScript("npc_big_ooze") { }

        struct npc_big_oozeAI : public ScriptedAI
        {
            npc_big_oozeAI(Creature* creature) : ScriptedAI(creature), instance(creature->GetInstanceScript())
            {
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                DoCast(me, SPELL_LARGE_OOZE_COMBINE, true);
                DoCast(me, SPELL_LARGE_OOZE_BUFF_COMBINE, true);
                DoCast(me, SPELL_RADIATING_OOZE, true);
                DoCast(me, SPELL_UNSTABLE_OOZE, true);
                DoCast(me, SPELL_GREEN_ABOMINATION_HITTIN__YA_PROC, true);
                events.ScheduleEvent(EVENT_STICKY_OOZE, 5000);
                // register in Rotface's summons - not summoned with Rotface as owner
                if (Creature* rotface = Unit::GetCreature(*me, instance->GetData64(DATA_ROTFACE)))
                    rotface->AI()->JustSummoned(me);
            }

            void JustDied(Unit* /*killer*/) override
            {
                if (Creature* rotface = Unit::GetCreature(*me, instance->GetData64(DATA_ROTFACE)))
                    rotface->AI()->SummonedCreatureDespawn(me);
                me->DespawnOrUnsummon();
            }

            void DoAction(int32 action) override
            {
                if (action == EVENT_STICKY_OOZE)
                {
                    events.CancelEvent(EVENT_STICKY_OOZE);
                    me->InterruptNonMeleeSpells(false);
                }
            }

            void UpdateAI(uint32 diff) override
            {
                if (InstanceScript* instance = me->GetInstanceScript())
                {
                    if (instance->GetBossState(DATA_ROTFACE) != IN_PROGRESS)
                    {
                        me->DespawnOrUnsummon();
                        return;
                    }
                }

                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_STICKY_OOZE:
                            // Set Rotface to original caster for this, no effect of sticky ooze will be triggered if ooze gets despawned
                            me->CastSpell(me->GetVictim(), SPELL_STICKY_OOZE, false, 0, 0, instance->GetData64(DATA_ROTFACE));
                            //DoCastVictim(SPELL_STICKY_OOZE);
                            events.ScheduleEvent(EVENT_STICKY_OOZE, 15000);
                        default:
                            break;
                    }
                }

                if (me->IsVisible())
                    DoMeleeAttackIfReady();
            }

        private:
            EventMap events;
            InstanceScript* instance;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetIcecrownCitadelAI<npc_big_oozeAI>(creature);
        }
};

class npc_precious_icc : public CreatureScript
{
    public:
        npc_precious_icc() : CreatureScript("npc_precious_icc") { }

        struct npc_precious_iccAI : public ScriptedAI
        {
            npc_precious_iccAI(Creature* creature) : ScriptedAI(creature)
            {
                _instance = creature->GetInstanceScript();
            }

            void Reset() override
            {
                _events.Reset();
                _events.ScheduleEvent(EVENT_DECIMATE, urand(20000, 25000));
                _events.ScheduleEvent(EVENT_SUMMON_ZOMBIES, urand(20000, 22000));
            }

            void JustSummoned(Creature* summon) override
            {
                summon->SetSpeed(MOVE_RUN, summon->GetSpeed(MOVE_WALK) / baseMoveSpeed[MOVE_RUN]);
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    summon->AI()->AttackStart(target);
                summon->CastSpell(summon, 65208, true); // Self Stun (3 secs)
            }

            void JustDied(Unit* /*killer*/) override
            {
                if (Creature* rotface = Unit::GetCreature(*me, _instance->GetData64(DATA_ROTFACE)))
                    if (rotface->IsAlive())
                        rotface->AI()->Talk(SAY_PRECIOUS_DIES);
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_DECIMATE:
                            DoCastVictim(SPELL_DECIMATE);
                            _events.ScheduleEvent(EVENT_DECIMATE, urand(20000, 25000));
                            break;
                        case EVENT_SUMMON_ZOMBIES:
                            Talk(EMOTE_PRECIOUS_ZOMBIES);
                            for (uint32 i = 0; i < 11; ++i)
                                DoCast(me, SPELL_AWAKEN_PLAGUED_ZOMBIES, true);
                            _events.ScheduleEvent(EVENT_SUMMON_ZOMBIES, urand(20000, 22000));
                            break;
                        default:
                            break;
                    }
                }

                if (!me->GetCurrentSpell(CURRENT_MELEE_SPELL))
                    DoCastVictim(SPELL_MORTAL_WOUND);

                DoMeleeAttackIfReady();
            }

        private:
            EventMap _events;
            InstanceScript* _instance;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetIcecrownCitadelAI<npc_precious_iccAI>(creature);
        }
};

class spell_awaken_plagued_zombies : public SpellScriptLoader
{
    public:
        spell_awaken_plagued_zombies() : SpellScriptLoader("spell_awaken_plagued_zombies") { }

        class spell_awaken_plagued_zombies_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_awaken_plagued_zombies_SpellScript);

            void TargetSelect(SpellDestination& dest)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                float dist = caster->GetExactDist2d(&dest._position);
                float angle = caster->GetRelativeAngle(&dest._position);

                caster->GetPosition(&dest._position);
                for (; dist > 0; dist -= 3.0f)
                    caster->MovePositionToFirstCollision(dest._position, 3.0f, angle);
            }

            void Register() override
            {
                OnDestinationTargetSelect += SpellDestinationTargetSelectFn(spell_awaken_plagued_zombies_SpellScript::TargetSelect, EFFECT_0, TARGET_DEST_CASTER_RADIUS);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_awaken_plagued_zombies_SpellScript();
        }
};

class spell_rotface_ooze_flood : public SpellScriptLoader
{
    public:
        spell_rotface_ooze_flood() : SpellScriptLoader("spell_rotface_ooze_flood") { }

        class spell_rotface_ooze_flood_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_ooze_flood_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (!GetHitUnit())
                    return;
                std::list<Creature*> triggers;
                GetHitUnit()->GetCreatureListWithEntryInGrid(triggers, GetHitUnit()->GetEntry(), 12.5f);

                if (triggers.empty())
                    return;

                triggers.sort(Trinity::ObjectDistanceOrderPred(GetHitUnit()));
                GetHitUnit()->CastSpell(triggers.back(), uint32(GetEffectValue()), false, nullptr, nullptr, GetOriginalCaster() ? GetOriginalCaster()->GetGUID() : 0);
            }

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                // get 2 targets except 2 nearest
                targets.sort(Trinity::ObjectDistanceOrderPred(GetCaster()));

                // .resize() runs pop_back();
                if (targets.size() > 5)
                    targets.resize(5);

                while (targets.size() > 2)
                    targets.pop_front();
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_rotface_ooze_flood_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_rotface_ooze_flood_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_rotface_ooze_flood_SpellScript();
        }
};

class spell_rotface_ooze_flood_damage : public SpellScriptLoader
{
    public:
        spell_rotface_ooze_flood_damage() : SpellScriptLoader("spell_rotface_ooze_flood_damage") { }

        class spell_rotface_ooze_flood_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_ooze_flood_damage_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                targets.remove_if([this](WorldObject* u)
                {
                    if (Unit* unit = u->ToUnit())
                        if (Aura* aura = unit->GetAura(GetSpellInfo()->Id))
                            return (aura->GetMaxDuration() - aura->GetDuration()) < 900;
                    return false;
                });
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_rotface_ooze_flood_damage_SpellScript::FilterTargets, EFFECT_ALL, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_rotface_ooze_flood_damage_SpellScript();
        }
};

class spell_rotface_mutated_infection : public SpellScriptLoader
{
    public:
        spell_rotface_mutated_infection() : SpellScriptLoader("spell_rotface_mutated_infection") { }

        class spell_rotface_mutated_infection_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_mutated_infection_SpellScript);

            bool Load() override
            {
                _target = nullptr;
                return true;
            }

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                // remove targets with this aura already
                // tank is not on this list
                targets.remove_if(Trinity::UnitAuraCheck(true, GetSpellInfo()->Id));
                if (targets.empty())
                    return;

                WorldObject* target = Trinity::Containers::SelectRandomContainerElement(targets);
                targets.clear();
                targets.push_back(target);
                _target = target;
            }

            void ReplaceTargets(std::list<WorldObject*>& targets)
            {
                targets.clear();
                if (_target)
                    targets.push_back(_target);
            }

            void NotifyTargets()
            {
                if (Creature* caster = GetCaster()->ToCreature())
                    if (Unit* target = GetHitUnit())
                        caster->AI()->Talk(EMOTE_MUTATED_INFECTION, target);
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_rotface_mutated_infection_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_rotface_mutated_infection_SpellScript::ReplaceTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_rotface_mutated_infection_SpellScript::ReplaceTargets, EFFECT_2, TARGET_UNIT_SRC_AREA_ENEMY);
                AfterHit += SpellHitFn(spell_rotface_mutated_infection_SpellScript::NotifyTargets);
            }

            WorldObject* _target;
        };

        class spell_rotface_mutated_infection_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_rotface_mutated_infection_AuraScript);

            void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes mode)
            {
                if (Unit* target = GetTarget())
                    if (target->GetInstanceScript() && target->GetInstanceScript()->GetBossState(DATA_ROTFACE) == IN_PROGRESS)
                        target->CastSpell(target, GetSpellInfo()->Effects[EFFECT_2].CalcValue(), true);
            }

            void Register() override
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_rotface_mutated_infection_AuraScript::HandleRemove, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_rotface_mutated_infection_SpellScript();
        }

        AuraScript* GetAuraScript() const override
        {
            return new spell_rotface_mutated_infection_AuraScript();
        }
};

class spell_rotface_little_ooze_combine : public SpellScriptLoader
{
    public:
        spell_rotface_little_ooze_combine() : SpellScriptLoader("spell_rotface_little_ooze_combine") { }

        class spell_rotface_little_ooze_combine_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_little_ooze_combine_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (!(GetHitCreature() && GetHitUnit()->IsAlive()))
                    return;

                GetCaster()->RemoveAurasDueToSpell(SPELL_LITTLE_OOZE_COMBINE);
                GetHitCreature()->RemoveAurasDueToSpell(SPELL_LITTLE_OOZE_COMBINE);
                GetHitCreature()->CastSpell(GetCaster(), SPELL_OOZE_MERGE, true);
                GetHitCreature()->DespawnOrUnsummon();
                GetCaster()->ToCreature()->DespawnOrUnsummon();
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_rotface_little_ooze_combine_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_rotface_little_ooze_combine_SpellScript();
        }
};

class spell_rotface_large_ooze_combine : public SpellScriptLoader
{
    public:
        spell_rotface_large_ooze_combine() : SpellScriptLoader("spell_rotface_large_ooze_combine") { }

        class spell_rotface_large_ooze_combine_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_large_ooze_combine_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (!(GetHitCreature() && GetHitCreature()->IsAlive()))
                    return;

                if (Aura* unstable = GetCaster()->GetAura(SPELL_UNSTABLE_OOZE))
                {
                    if (Aura* targetAura = GetHitCreature()->GetAura(SPELL_UNSTABLE_OOZE))
                        unstable->ModStackAmount(targetAura->GetStackAmount());
                    else
                        unstable->ModStackAmount(1);

                    GetCaster()->SetObjectScale(0.8f + 0.2f * unstable->GetStackAmount()); // Can't find any related auras :(

                    if (unstable->GetStackAmount() >= 4)
                        GetCaster()->CastSpell(GetCaster(), SPELL_VERTEX_COLOR_BRIGHT_RED, true);

                    // explode!
                    if (unstable->GetStackAmount() >= 5)
                    {
                        GetCaster()->RemoveAurasDueToSpell(SPELL_LARGE_OOZE_BUFF_COMBINE);
                        GetCaster()->RemoveAurasDueToSpell(SPELL_LARGE_OOZE_COMBINE);
                        if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                            if (Creature* rotface = Unit::GetCreature(*GetCaster(), instance->GetData64(DATA_ROTFACE)))
                                if (rotface->IsAlive())
                                {
                                    rotface->AI()->Talk(EMOTE_UNSTABLE_EXPLOSION);
                                    rotface->AI()->Talk(SAY_UNSTABLE_EXPLOSION);
                                }

                        if (Creature* cre = GetCaster()->ToCreature())
                            cre->AI()->DoAction(EVENT_STICKY_OOZE);
                        GetCaster()->CastSpell(GetCaster(), SPELL_UNSTABLE_OOZE_EXPLOSION, false, nullptr, nullptr, GetCaster()->GetGUID());
                        GetCaster()->GetMap()->SetWorldState(WORLDSTATE_DANCES_WITH_OOZES, 0);
                    }
                }

                // just for safety
                GetHitCreature()->RemoveAurasDueToSpell(SPELL_LARGE_OOZE_BUFF_COMBINE);
                GetHitCreature()->RemoveAurasDueToSpell(SPELL_LARGE_OOZE_COMBINE);
                GetHitCreature()->DespawnOrUnsummon();
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_rotface_large_ooze_combine_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_rotface_large_ooze_combine_SpellScript();
        }
};

class spell_rotface_large_ooze_buff_combine : public SpellScriptLoader
{
    public:
        spell_rotface_large_ooze_buff_combine() : SpellScriptLoader("spell_rotface_large_ooze_buff_combine") { }

        class spell_rotface_large_ooze_buff_combine_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_large_ooze_buff_combine_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (!(GetHitCreature() && GetHitCreature()->IsAlive()))
                    return;

                if (Aura* unstable = GetCaster()->GetAura(SPELL_UNSTABLE_OOZE))
                {
                    unstable->ModStackAmount(1);

                    GetCaster()->SetObjectScale(0.8f + 0.2f * unstable->GetStackAmount()); // Can't find any related auras :(

                    if (unstable->GetStackAmount() >= 4)
                        GetCaster()->CastSpell(GetCaster(), SPELL_VERTEX_COLOR_BRIGHT_RED, true);

                    // explode!
                    if (unstable->GetStackAmount() >= 5)
                    {
                        GetCaster()->RemoveAurasDueToSpell(SPELL_LARGE_OOZE_BUFF_COMBINE);
                        GetCaster()->RemoveAurasDueToSpell(SPELL_LARGE_OOZE_COMBINE);
                        if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                            if (Creature* rotface = Unit::GetCreature(*GetCaster(), instance->GetData64(DATA_ROTFACE)))
                                if (rotface->IsAlive())
                                {
                                    rotface->AI()->Talk(EMOTE_UNSTABLE_EXPLOSION);
                                    rotface->AI()->Talk(SAY_UNSTABLE_EXPLOSION);
                                }

                        if (Creature* cre = GetCaster()->ToCreature())
                            cre->AI()->DoAction(EVENT_STICKY_OOZE);
                        GetCaster()->CastSpell(GetCaster(), SPELL_UNSTABLE_OOZE_EXPLOSION, false, nullptr, nullptr, GetCaster()->GetGUID());
                        GetCaster()->GetMap()->SetWorldState(WORLDSTATE_DANCES_WITH_OOZES, 0);
                    }
                }

                GetHitCreature()->DespawnOrUnsummon();
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_rotface_large_ooze_buff_combine_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_rotface_large_ooze_buff_combine_SpellScript();
        }
};

class spell_rotface_unstable_ooze_explosion_init : public SpellScriptLoader
{
    public:
        spell_rotface_unstable_ooze_explosion_init() : SpellScriptLoader("spell_rotface_unstable_ooze_explosion_init") { }

        class spell_rotface_unstable_ooze_explosion_init_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_unstable_ooze_explosion_init_SpellScript);

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_UNSTABLE_OOZE_EXPLOSION_TRIGGER))
                    return false;
                return true;
            }

            void HandleCast(SpellEffIndex effIndex)
            {
                PreventHitEffect(effIndex);
                if (!GetHitUnit())
                    return;

                float x, y, z;
                GetHitUnit()->GetPosition(x, y, z);
                Creature* dummy = GetCaster()->SummonCreature(NPC_UNSTABLE_EXPLOSION_STALKER, x, y, z, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 60000);
                GetCaster()->CastSpell(dummy, SPELL_UNSTABLE_OOZE_EXPLOSION_TRIGGER, true);
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_rotface_unstable_ooze_explosion_init_SpellScript::HandleCast, EFFECT_0, SPELL_EFFECT_FORCE_CAST);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_rotface_unstable_ooze_explosion_init_SpellScript();
        }
};

class spell_rotface_unstable_ooze_explosion : public SpellScriptLoader
{
    public:
        spell_rotface_unstable_ooze_explosion() : SpellScriptLoader("spell_rotface_unstable_ooze_explosion") { }

        class spell_rotface_unstable_ooze_explosion_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_unstable_ooze_explosion_SpellScript);

            void CheckTarget(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(EFFECT_0);
                if (!GetExplTargetDest())
                    return;

                uint32 triggered_spell_id = GetSpellInfo()->Effects[effIndex].TriggerSpell;

                float x, y, z;
                GetExplTargetDest()->GetPosition(x, y, z);
                // let Rotface handle the cast - caster dies before this executes
                if (InstanceScript* script = GetCaster()->GetInstanceScript())
                    if (Creature* rotface = script->instance->GetCreature(script->GetData64(DATA_ROTFACE)))
                        rotface->CastSpell(x, y, z, triggered_spell_id, true, nullptr, nullptr, GetCaster()->GetGUID());
            }

            void Register() override
            {
                OnEffectHit += SpellEffectFn(spell_rotface_unstable_ooze_explosion_SpellScript::CheckTarget, EFFECT_0, SPELL_EFFECT_TRIGGER_MISSILE);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_rotface_unstable_ooze_explosion_SpellScript();
        }
};

class spell_rotface_unstable_ooze_explosion_suicide : public SpellScriptLoader
{
    public:
        spell_rotface_unstable_ooze_explosion_suicide() : SpellScriptLoader("spell_rotface_unstable_ooze_explosion_suicide") { }

        class spell_rotface_unstable_ooze_explosion_suicide_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_rotface_unstable_ooze_explosion_suicide_AuraScript);

            void DespawnSelf(AuraEffect const* /*aurEff*/)
            {
                PreventDefaultAction();
                Unit* target = GetTarget();
                if (target->GetTypeId() != TYPEID_UNIT)
                    return;

                target->RemoveAllAuras();
                target->SetVisible(false);
                target->ToCreature()->DespawnOrUnsummon(60000);
            }

            void Register() override
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_rotface_unstable_ooze_explosion_suicide_AuraScript::DespawnSelf, EFFECT_2, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_rotface_unstable_ooze_explosion_suicide_AuraScript();
        }
};

class spell_rotface_vile_gas_trigger : public SpellScriptLoader
{
    public:
        spell_rotface_vile_gas_trigger() : SpellScriptLoader("spell_rotface_vile_gas_trigger") { }

        class spell_rotface_vile_gas_trigger_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_vile_gas_trigger_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                targets.sort(Trinity::ObjectDistanceOrderPred(GetCaster()));
                if (targets.empty())
                    return;

                std::list<WorldObject*> ranged, melee;
                std::list<WorldObject*>::iterator itr = targets.begin();
                while (itr != targets.end() && (*itr)->GetDistance(GetCaster()) < 5.0f)
                {
                    melee.push_back((*itr)->ToUnit());
                    ++itr;
                }

                while (itr != targets.end())
                {
                    ranged.push_back((*itr)->ToUnit());
                    ++itr;
                }

                uint32 minTargets = GetCaster()->GetMap()->Is25ManRaid() ? 8 : 3;
                while (ranged.size() < minTargets)
                {
                    if (melee.empty())
                        break;

                    WorldObject* target = Trinity::Containers::SelectRandomContainerElement(melee);
                    ranged.push_back(target);
                    melee.remove(target);
                }

                if (!ranged.empty())
                    Trinity::Containers::RandomResizeList(ranged, GetCaster()->GetMap()->Is25ManRaid() ? 3 : 1);

                targets.swap(ranged);
            }

            void HandleDummy(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                GetCaster()->CastSpell(GetHitUnit(), SPELL_VILE_GAS_TRIGGER_SUMMON);
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_rotface_vile_gas_trigger_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnEffectHitTarget += SpellEffectFn(spell_rotface_vile_gas_trigger_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_rotface_vile_gas_trigger_SpellScript();
        }
};

void AddSC_boss_rotface()
{
    new boss_rotface();
    new npc_little_ooze();
    new npc_big_ooze();
    new npc_precious_icc();
    new spell_awaken_plagued_zombies();
    new spell_rotface_ooze_flood();
    new spell_rotface_ooze_flood_damage();
    new spell_rotface_mutated_infection();
    new spell_rotface_little_ooze_combine();
    new spell_rotface_large_ooze_combine();
    new spell_rotface_large_ooze_buff_combine();
    new spell_rotface_unstable_ooze_explosion_init();
    new spell_rotface_unstable_ooze_explosion();
    new spell_rotface_unstable_ooze_explosion_suicide();
    new spell_rotface_vile_gas_trigger();
}
