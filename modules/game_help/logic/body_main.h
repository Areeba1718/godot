
#ifndef BODY_MAIN_H
#define BODY_MAIN_H
#include "scene/resources/packed_scene.h"
#include "scene/animation/animation_player.h"
#include "scene/animation/animation_tree.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/3d/physics/character_body_3d.h"
#include "scene/3d/physics/collision_shape_3d.h"
#include "body_part.h"
#include "animation_help.h"
#include "body_animator.h"


#include "modules/limboai/bt/bt_player.h"
#include "modules/limboai/bt/tasks/decorators/bt_new_scope.h"

// 身体的插槽信息
class BodySocket
{
    Transform3D localPose;
    Transform3D globalPose;

    void on_bone_pose_update(Skeleton3D *p_skeleton, int p_bone_index)
    {
        globalPose = p_skeleton->get_bone_global_pose(p_bone_index) * localPose;
    }

};
// 身体主要部件部分
class CharacterBodyMain : public CharacterBody3D {
    GDCLASS(CharacterBodyMain, CharacterBody3D);
    static void _bind_methods();

public:
    // 初始化身體
    void init_main_body(String p_skeleton_file_path,StringName p_animation_group);
    void clear_all();
    CharacterBodyMain();
    ~CharacterBodyMain();

public:
	void set_behavior_tree(const Ref<BehaviorTree> &p_tree)
    {
        get_bt_player()->set_behavior_tree(p_tree);
    }
	Ref<BehaviorTree> get_behavior_tree()  { return get_bt_player()->get_behavior_tree(); };

	void set_blackboard_plan(const Ref<BlackboardPlan> &p_plan)
    {
        get_bt_player()->set_blackboard_plan(p_plan);
    }
	Ref<BlackboardPlan> get_blackboard_plan() { return get_bt_player()->get_blackboard_plan(); }

	void set_update_mode(int p_mode)
    {
        get_bt_player()->set_update_mode((BTPlayer::UpdateMode)p_mode);
    }
	int get_update_mode() { return (int)(get_bt_player()->get_update_mode()); }

	Ref<Blackboard> get_blackboard()  { return player_blackboard; }

    void set_skeleton(Skeleton3D *p_skeleton) { }
    Skeleton3D *get_skeleton() { return skeleton; }
    // 设置黑板
	void set_blackboard(const Ref<Blackboard> &p_blackboard) 
    { 
        player_blackboard = p_blackboard; 
        get_bt_player()->get_blackboard()->set_parent(p_blackboard);
        if(btSkillPlayer != nullptr)
        {
            btSkillPlayer->get_blackboard()->set_parent(player_blackboard); 
        }
    }

	void restart()
    {
        get_bt_player()->restart();
    }
	int get_last_status() { return get_bt_player()->get_last_status(); }

	Ref<BTTask> get_tree_instance() { return get_bt_player()->get_tree_instance(); }

    void set_controller(const Ref<class CharacterController> &p_controller);
    Ref<class CharacterController> get_controller();

    void set_main_shape(const Ref<Shape3D>& p_shape) {
        if(mainShape != nullptr)
        {
            mainShape->set_shape(p_shape);
        }
    }
    Ref<Shape3D> get_main_shape()
     {
        if(mainShape == nullptr)
        {
            return mainShape->get_shape();
        }
        return mainShape; 
    }
public:
    // 初始化身體分組信息
    void init_body_part_array(const Array& p_part_array);
    // 身體部位
    void set_body_part(const Dictionary& part);
    Dictionary get_body_part();
    // 技能相关
public:
    bool play_skill(String p_skill_name);
    void stop_skill();

    // 动画相关
public:
    void set_animator(const Ref<CharacterAnimator> &p_animator)
    {

    }

    Ref<CharacterAnimator> get_animator()
    {
        return animator;
    }
public:
    
	virtual void input(const Ref<InputEvent> &p_event) override
    {
    }
	virtual void shortcut_input(const Ref<InputEvent> &p_key_event) override
    {

    }
	virtual void unhandled_input(const Ref<InputEvent> &p_event)override
    {

    }
	virtual void unhandled_key_input(const Ref<InputEvent> &p_key_event)override
    {

    }
protected:
    void load_skeleton();
    void load_mesh(const StringName& part_name,String p_mesh_file_path);
    BTPlayer * get_bt_player();
    void _stop_skill()
    {
        if(btSkillPlayer != nullptr)
        {
            memdelete(btSkillPlayer);
            btSkillPlayer = nullptr;
        }
    }
    
protected:
    void behavior_tree_finished(int last_status);
    void behavior_tree_update(int last_status);

    
    void skill_tree_finished(int last_status);
    void skill_tree_update(int last_status);


protected:
    Skeleton3D *skeleton = nullptr;
    AnimationPlayer *player = nullptr;
    CollisionShape3D * mainShape = nullptr;
    mutable BTPlayer *btPlayer = nullptr;
    bool is_skill_stop = false;
    // 技能播放器
    mutable BTPlayer *btSkillPlayer = nullptr;
    // 角色自己的黑板
    Ref<Blackboard> player_blackboard;
    // 骨架配置文件
    String skeleton_res;
    // 动画组配置
    String animation_group;
    // 身體部件列表
    PackedStringArray   partList;
    // 插槽信息
    HashMap<StringName,BodySocket> socket;
    // 身体部件信息
    HashMap<StringName,Ref<CharacterBodyPartInstane>> bodyPart;
    Ref<CharacterAnimator>    animator;
    Ref<class CharacterController>  controller;
    // 初始化数据
    Dictionary init_data;
    


};

class CharacterBodyMain;
// 角色控制器
class CharacterController : public Resource
{
    GDCLASS(CharacterController, Resource);
    static void _bind_methods()
    {
        ClassDB::bind_method(D_METHOD("set_blackboardPlan", "blackboardplan"), &CharacterController::set_blackboardPlan);
        ClassDB::bind_method(D_METHOD("get_blackboardPlan"), &CharacterController::get_blackboardPlan);

        ClassDB::bind_method(D_METHOD("set_load_test_id", "id"), &CharacterController::set_load_test_id);
        ClassDB::bind_method(D_METHOD("get_load_test_id"), &CharacterController::get_load_test_id);

        ClassDB::bind_method(D_METHOD("set_load_test_player"), &CharacterController::set_load_test_player);
        ClassDB::bind_method(D_METHOD("get_load_test_player"), &CharacterController::get_load_test_player);

        ClassDB::bind_method(D_METHOD("set_bt_load_test_id", "id"), &CharacterController::set_bt_load_test_id);
        ClassDB::bind_method(D_METHOD("get_bt_load_test_id"), &CharacterController::get_bt_load_test_id);


        ClassDB::bind_method(D_METHOD("load_test"), &CharacterController::load_test);
        ClassDB::bind_method(D_METHOD("log_player"), &CharacterController::log_player);

        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "blackboard_plan", PROPERTY_HINT_RESOURCE_TYPE, "BlackboardPlan"), "set_blackboardPlan", "get_blackboardPlan");

        ADD_GROUP("Load Test", "load_test_");
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "load_test_player", PROPERTY_HINT_NODE_TYPE, "CharacterBodyMain"), "set_load_test_player", "get_load_test_player");

        ADD_PROPERTY(PropertyInfo(Variant::INT, "load_test_id"), "set_load_test_id", "get_load_test_id");
        // 增加一个按钮属性
        ADD_PROPERTY(PropertyInfo(Variant::INT, "load_test_bt",PROPERTY_HINT_BUTTON,"#FF22AA;Load Test;load_test()"), "set_bt_load_test_id", "get_bt_load_test_id");
        
        GDVIRTUAL_BIND(_player_startup, "player","init_data");
        GDVIRTUAL_BIND(_player_pre_update_player,"player");
        GDVIRTUAL_BIND(_player_update_player, "player");
        GDVIRTUAL_BIND(_player_post_update_player, "player");
        GDVIRTUAL_BIND(_player_on_restart, "player");
        GDVIRTUAL_BIND(_player_on_dead, "player");
        GDVIRTUAL_BIND(_player_stop_player, "player");

        
        GDVIRTUAL_BIND(_player_input, "player", "event");
        GDVIRTUAL_BIND(_player_shortcut_input, "player", "event");
        GDVIRTUAL_BIND(_player_unhandled_input, "player", "event");
        GDVIRTUAL_BIND(_player_unhandled_key_input, "player", "event");
    }
public:
    bool is_input()
    {
        return GDVIRTUAL_IS_OVERRIDDEN(_player_input);
    }
    bool is_shortcut_input()
    {
        return GDVIRTUAL_IS_OVERRIDDEN(_player_shortcut_input);
    }
    bool is_unhandled_input()
    {
        return GDVIRTUAL_IS_OVERRIDDEN(_player_unhandled_input);
    }
    bool is_unhandled_key_input()
    {
        return GDVIRTUAL_IS_OVERRIDDEN(_player_unhandled_key_input);
    }
    void set_blackboardPlan(Ref<BlackboardPlan> p_blackboardplan)
    {
        blackboard_plan = p_blackboardplan;
    }
    Ref<BlackboardPlan> get_blackboardPlan()
    {
        return blackboard_plan;
    }
    int load_test_id = 0;
    void set_load_test_id(int p_id)
    {
        load_test_id = p_id;
    }
    int get_load_test_id()
    {
        return load_test_id;
    }
    WeakRef load_test_player;

    void set_load_test_player(CharacterBodyMain* p_player)
    {
        load_test_player.set_obj(p_player);
    }
    CharacterBodyMain* get_load_test_player()
    {
        Object* obj = load_test_player.get_ref();
        if (obj)
        {
            return Object::cast_to<CharacterBodyMain>(obj);
        }
        return nullptr;
    }

    void set_bt_load_test_id(int p_id)
    {}
    int get_bt_load_test_id()
    {
        return 0;
    }
    void load_test();
    void log_player();
public:
    void startup(CharacterBodyMain* p_player,const Dictionary& p_init_data)
    {

		GDVIRTUAL_CALL(_player_startup, p_player,p_init_data);
    }
    void pre_update_player(CharacterBodyMain* p_player)
    {
		GDVIRTUAL_CALL(_player_pre_update_player, p_player);

    }

    void update_player(CharacterBodyMain* p_player)
    {
		GDVIRTUAL_CALL(_player_update_player, p_player);

    }

    void post_update_player(CharacterBodyMain* p_player)
    {
		GDVIRTUAL_CALL(_player_post_update_player, p_player);

    }
    void on_restart(CharacterBodyMain* p_player)
    {
		GDVIRTUAL_CALL(_player_on_restart, p_player);
        
    }
    void on_dead(CharacterBodyMain* p_player)
    {
		GDVIRTUAL_CALL(_player_on_dead, p_player);

    }
    void stop_player(CharacterBodyMain* p_player)
    {

		GDVIRTUAL_CALL(_player_stop_player, p_player);
    }
	void input(CharacterBodyMain* p_player,const Ref<InputEvent> &p_event) 
    {
		GDVIRTUAL_CALL(_player_input, p_player,p_event);
    }
	virtual void shortcut_input(CharacterBodyMain* p_player,const Ref<InputEvent> &p_key_event) 
    {
		GDVIRTUAL_CALL(_player_shortcut_input, p_player,p_key_event);

    }
	virtual void unhandled_input(CharacterBodyMain* p_player,const Ref<InputEvent> &p_event)
    {
		GDVIRTUAL_CALL(_player_unhandled_input, p_player,p_event);

    }
	virtual void unhandled_key_input(CharacterBodyMain* p_player,const Ref<InputEvent> &p_key_event)
    {
		GDVIRTUAL_CALL(_player_unhandled_key_input, p_player,p_key_event);
    }


	GDVIRTUAL2(_player_startup, CharacterBodyMain*,Dictionary);
	GDVIRTUAL1(_player_pre_update_player, CharacterBodyMain*);
	GDVIRTUAL1(_player_update_player, CharacterBodyMain*);
	GDVIRTUAL1(_player_post_update_player, CharacterBodyMain*);
	GDVIRTUAL1(_player_on_restart, CharacterBodyMain*);
	GDVIRTUAL1(_player_on_dead, CharacterBodyMain*);
	GDVIRTUAL1(_player_stop_player,CharacterBodyMain*);


    
	GDVIRTUAL2(_player_input, CharacterBodyMain*,Ref<InputEvent>);
	GDVIRTUAL2(_player_shortcut_input, CharacterBodyMain*,Ref<InputEvent>);
	GDVIRTUAL2(_player_unhandled_input, CharacterBodyMain*,Ref<InputEvent>);
	GDVIRTUAL2(_player_unhandled_key_input, CharacterBodyMain*,Ref<InputEvent>);

    Ref<BlackboardPlan> blackboard_plan;

};


#endif