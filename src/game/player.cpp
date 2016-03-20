#include "player.h"
#include "awk.h"
#include "data/components.h"
#include "entities.h"
#include "render/render.h"
#include "input.h"
#include "common.h"
#include "console.h"
#include "bullet/src/BulletCollision/NarrowPhaseCollision/btRaycastCallback.h"
#include "physics.h"
#include "asset/font.h"
#include "asset/lookup.h"
#include "asset/mesh.h"
#include "asset/shader.h"
#include "asset/texture.h"
#include "asset/animation.h"
#include "asset/armature.h"
#include "strings.h"
#include "ease.h"
#include "render/skinned_model.h"
#include "render/views.h"
#include "audio.h"
#include "asset/Wwise_IDs.h"
#include "game.h"
#include "minion.h"
#include "walker.h"
#include "data/animator.h"
#include "mersenne/mersenne-twister.h"
#include "parkour.h"

namespace VI
{

#define fov_initial (80.0f * PI * 0.5f / 180.0f)
#define zoom_ratio 0.5f
#define zoom_speed (1.0f / 0.1f)
#define speed_mouse 0.0025f
#define fov_zoom (fov_initial * zoom_ratio)
#define speed_mouse_zoom (speed_mouse * zoom_ratio)
#define speed_joystick 5.0f
#define speed_joystick_zoom (speed_joystick * zoom_ratio)
#define attach_speed 5.0f
#define max_attach_time 0.35f
#define rotation_speed 20.0f
#define msg_time 0.75f

b8 flash_function(r32 time)
{
	return (b8)((s32)(time * 16.0f) % 2);
}

void draw_indicator(const RenderParams& params, const Vec3& pos, const Vec4& color, b8 flash = false)
{
	b8 show;
	if (flash)
		show = flash_function(params.sync->time.total);
	else
		show = true;
	if (show)
	{
		const Rect2& viewport = params.camera->viewport;
		Vec2 screen_pos;
		b8 on_screen = UI::project(params, pos, &screen_pos);
		Vec2 center = viewport.size * 0.5f;
		Vec2 offset = screen_pos - center;
		if (!on_screen)
			offset *= -1.0f;

		r32 radius = fmin(viewport.size.x, viewport.size.y) * 0.95f * 0.5f;

		r32 offset_length = offset.length();
		if (offset_length > radius || (offset_length > 0.0f && !on_screen))
		{
			offset *= radius / offset_length;
			UI::triangle(params, { center + offset, Vec2(32) * UI::scale }, color, atan2f(offset.y, offset.x) + PI * -0.5f);
		}
		else
			UI::centered_border(params, { center + offset, Vec2(32) * UI::scale }, 4, color, PI * 0.25f);
	}
}

PinArray<LocalPlayer, MAX_PLAYERS> LocalPlayer::list;

LocalPlayer::LocalPlayer(PlayerManager* m, u8 g)
	: gamepad(g),
	manager(m),
	pause(),
	camera(),
	credits_text(),
	msg_text(),
	msg_timer(msg_time),
	menu(),
	revision(),
	options_menu(),
	upgrading()
{
	sprintf(manager.ref()->username, _(strings::player), gamepad);

	msg_text.font = Asset::Font::lowpoly;
	msg_text.size = 18.0f;
	msg_text.color = UI::default_color;
	msg_text.anchor_x = UIText::Anchor::Center;
	msg_text.anchor_y = UIText::Anchor::Center;

	credits_text.font = Asset::Font::lowpoly;
	credits_text.size = 18.0f;
	credits_text.color = UI::default_color;
	credits_text.anchor_x = UIText::Anchor::Min;
	credits_text.anchor_y = UIText::Anchor::Min;

	m->spawn.link<LocalPlayer, &LocalPlayer::spawn>(this);
}

LocalPlayer::UIMode LocalPlayer::ui_mode() const
{
	if (pause)
		return UIMode::Pause;
	else if (upgrading)
		return UIMode::Upgrading;
	else if (manager.ref()->entity.ref() || NoclipControl::list.count() > 0)
		return UIMode::Default;
	else
		return UIMode::Spawning;
}

void LocalPlayer::msg(const char* msg)
{
	msg_text.text(msg);
	msg_timer = 0.0f;
}

void LocalPlayer::ensure_camera(const Update& u, b8 active)
{
	if (active && !camera && !manager.ref()->entity.ref())
	{
		camera = Camera::add();
		camera->fog = Game::data.mode == Game::Mode::Parkour;
		camera->team = (u8)manager.ref()->team.ref()->team();
		camera->mask = 1 << camera->team;
		s32 player_count = list.count();
		Camera::ViewportBlueprint* viewports = Camera::viewport_blueprints[player_count - 1];
		Camera::ViewportBlueprint* blueprint = &viewports[gamepad];

		camera->viewport =
		{
			Vec2((s32)(blueprint->x * (r32)u.input->width), (s32)(blueprint->y * (r32)u.input->height)),
			Vec2((s32)(blueprint->w * (r32)u.input->width), (s32)(blueprint->h * (r32)u.input->height)),
		};
		r32 aspect = camera->viewport.size.y == 0 ? 1 : (r32)camera->viewport.size.x / (r32)camera->viewport.size.y;
		camera->perspective(60.0f * PI * 0.5f / 180.0f, aspect, 1.0f, Skybox::far_plane * 2.0f);
		Quat rot;
		map_view.ref()->absolute(&camera->pos, &rot);
		camera->rot = Quat::look(rot * Vec3(0, -1, 0));
	}
	else if (camera && (!active || manager.ref()->entity.ref()))
	{
		camera->remove();
		camera = nullptr;
	}
}

void LocalPlayer::update(const Update& u)
{
	if (Console::visible)
		return;

	credits_text.text(_(strings::credits), manager.ref()->credits);

	if (msg_timer < msg_time)
		msg_timer += u.time.delta;

	if (manager.ref()->entity.ref())
		manager.ref()->entity.ref()->get<LocalPlayerControl>()->enable_input = ui_mode() == UIMode::Default;

	// close/open pause menu if needed
	{
		b8 pause_hit = u.input->get(Game::bindings.pause, gamepad) && !u.last_input->get(Game::bindings.pause, gamepad);
		if (pause)
		{
			if (!options_menu && pause_hit)
				pause = false;
		}
		else
		{
			if (!upgrading && pause_hit)
				pause = true;
		}
	}

	switch (ui_mode())
	{
		case UIMode::Default:
		{
			// nothing going on
			ensure_camera(u, false);
			if (manager.ref()->entity.ref())
				manager.ref()->entity.ref()->get<LocalPlayerControl>()->enable_input = true;
			const Settings& settings = Loader::settings();
			if (Game::data.mode == Game::Mode::Multiplayer)
			{
				if (u.input->get(settings.bindings.menu, gamepad) && !u.last_input->get(settings.bindings.menu, gamepad))
					upgrading = true;
			}

			break;
		}
		case UIMode::Upgrading:
		{
			ensure_camera(u, true);
			menu.start(u, gamepad);
			Rect2& viewport = camera ? camera->viewport : manager.ref()->entity.ref()->get<LocalPlayerControl>()->camera->viewport;

			const Settings& settings = Loader::settings();
			if ((u.input->get(settings.bindings.menu, gamepad) && !u.last_input->get(settings.bindings.menu, gamepad))
				|| (u.input->get(Game::bindings.cancel, gamepad) && !u.last_input->get(Game::bindings.cancel, gamepad)))
				upgrading = false;

			// do menu items
			Vec2 pos = Vec2(viewport.size.x * 0.5f + (MENU_ITEM_WIDTH * -0.5f), viewport.size.y * 0.75f);
			if (menu.item(u, &pos, _(strings::close)))
				upgrading = false;

			b8 can_buy_new_abilities = false;
			for (s32 i = 0; i < ABILITY_COUNT; i++)
			{
				if (manager.ref()->abilities[i].ability == Ability::None)
				{
					can_buy_new_abilities = true;
					break;
				}
			}

			// new abilities
			for (s32 i = 0; i < (s32)Ability::count; i++)
			{
				Ability ability = (Ability)i;

				// check if we've already bought this ability
				b8 already_bought = false;
				for (s32 i = 0; i < ABILITY_COUNT; i++)
				{
					if (manager.ref()->abilities[i].ability == ability)
					{
						already_bought = true;

						const AbilitySlot& slot = manager.ref()->abilities[i];
						if (slot.ability != Ability::None && slot.can_upgrade())
						{
							u16 cost = slot.upgrade_cost();
							char cost_str[255];
							sprintf(cost_str, _(strings::credits), cost);

							const AbilitySlot::Info& info = AbilitySlot::info[(s32)slot.ability];
							char upgrade_str[255];
							sprintf(upgrade_str, _(strings::upgrade), _(info.name));

							if (menu.item(u, &pos, upgrade_str, cost_str, manager.ref()->credits < cost, info.icon))
								manager.ref()->upgrade(slot.ability);
						}
						break;
					}
				}

				// allow buying new abilities
				if (!already_bought && can_buy_new_abilities)
				{
					const AbilitySlot::Info& info = AbilitySlot::info[(s32)ability];

					u16 cost = info.upgrade_cost[0];
					char cost_str[255];
					sprintf(cost_str, _(strings::credits), cost);
					if (menu.item(u, &pos, _(info.name), cost_str, manager.ref()->credits < cost, info.icon))
						manager.ref()->upgrade((Ability)i);
				}
			}

			menu.end();
			break;
		}
		case UIMode::Pause:
		{
			ensure_camera(u, true);
			Rect2& viewport = camera ? camera->viewport : manager.ref()->entity.ref()->get<LocalPlayerControl>()->camera->viewport;
			menu.start(u, gamepad);

			if (options_menu)
			{
				Vec2 pos = Vec2(0, viewport.size.y * 0.5f + Menu::options_height() * 0.5f);
				if (!Menu::options(u, gamepad, &menu, &pos))
					options_menu = false;
			}
			else
			{
				Vec2 pos = Vec2(0, viewport.size.y * 0.5f + UIMenu::height(3) * 0.5f);
				if (menu.item(u, &pos, _(strings::resume)))
					pause = false;
				if (menu.item(u, &pos, _(strings::options)))
					options_menu = true;
				if (menu.item(u, &pos, _(strings::main_menu)))
					Menu::title();
			}

			menu.end();

			break;
		}
		case UIMode::Spawning:
		{
			// Player is currently dead
			ensure_camera(u, true);

			break;
		}
	}
}

void LocalPlayer::spawn()
{
	if (NoclipControl::list.count() > 0)
		return;

	Vec3 pos;
	Quat rot;
	manager.ref()->team.ref()->player_spawn.ref()->absolute(&pos, &rot);
	Vec3 dir = rot * Vec3(0, 0, 1);
	r32 angle = atan2f(dir.x, dir.z);

	Entity* spawned;

	if (Game::data.mode == Game::Mode::Multiplayer)
	{
		// Spawn AWK
		pos += Quat::euler(0, angle + (gamepad * PI * 0.5f), 0) * Vec3(0, 0, PLAYER_SPAWN_RADIUS); // spawn it around the edges
		spawned = World::create<AwkEntity>(manager.ref()->team.ref()->team());
	}
	else // parkour mode
	{
		// Spawn traceur
		spawned = World::create<Traceur>(pos, Quat::euler(0, angle, 0), manager.ref()->team.ref()->team());
	}

	spawned->get<Transform>()->absolute(pos, rot);
	spawned->add<PlayerCommon>(manager.ref());
	manager.ref()->entity = spawned;

	LocalPlayerControl* control = spawned->add<LocalPlayerControl>(gamepad);
	control->player = this;
	control->angle_horizontal = angle;
}

void LocalPlayer::draw_alpha(const RenderParams& params) const
{
	if (params.camera != camera && (!manager.ref()->entity.ref() || params.camera != manager.ref()->entity.ref()->get<LocalPlayerControl>()->camera))
		return;

	const r32 line_thickness = 2.0f * UI::scale;
	const r32 padding = 6.0f * UI::scale;

	const Rect2& vp = params.camera->viewport;

	// message
	if (msg_timer < msg_time)
	{
		r32 last_timer = msg_timer;
		b8 flash = flash_function(msg_timer);
		b8 last_flash = flash_function(msg_timer - params.sync->time.delta);
		if (flash)
		{
			Vec2 pos = params.camera->viewport.size * Vec2(0.5f, 0.6f);
			Rect2 box = msg_text.rect(pos).outset(6 * UI::scale);
			UI::box(params, box, UI::background_color);
			msg_text.draw(params, pos);
			if (!last_flash)
				Audio::post_global_event(AK::EVENTS::PLAY_BEEP_GOOD);
		}
	}

	if (Game::data.mode == Game::Mode::Multiplayer)
	{
		Vec2 pos = vp.size * Vec2(0.1f, 0.12f);
		UI::box(params, credits_text.rect(pos).outset(6 * UI::scale), UI::background_color);
		credits_text.draw(params, pos);
	}

	switch (ui_mode())
	{
		case UIMode::Default:
		{
			if (manager.ref()->upgrade_available())
			{
				// show upgrade prompt
				UIText text;
				text.font = Asset::Font::lowpoly;
				text.anchor_x = UIText::Anchor::Min;
				text.anchor_y = UIText::Anchor::Min;
				text.size = 16.0f;

				text.color = UI::default_color;
				text.text(_(strings::upgrade_prompt));
				Vec2 p = vp.size * Vec2(0.2f, 0.12f);
				UI::box(params, text.rect(p).outset(8.0f * UI::scale), UI::background_color);
				text.draw(params, p);
			}
			break;
		}
		case UIMode::Pause:
		case UIMode::Upgrading:
		{
			menu.draw_alpha(params);
			break;
		}
		case UIMode::Spawning:
		{
			// player is dead

			b8 show_spawning = true;
			if (Game::data.mode == Game::Mode::Multiplayer && Team::game_over())
			{
				// we lost, we're not spawning again
				show_spawning = false;
			}

			if (show_spawning)
			{
				// Player is currently dead
				UIText text;
				text.font = Asset::Font::lowpoly;

				text.wrap_width = MENU_ITEM_WIDTH;
				text.anchor_x = text.anchor_y = UIText::Anchor::Center;
				text.color = UI::default_color;
				text.text(_(strings::spawning), (s32)manager.ref()->spawn_timer + 1);
				Vec2 p = vp.size * Vec2(0.5f);
				UI::box(params, text.rect(p).outset(8.0f * UI::scale), UI::background_color);
				text.draw(params, p);
			}

			break;
		}
	}

	if (Game::data.mode == Game::Mode::Multiplayer && Team::game_over())
	{
		// show victory/defeat message
		UIText text;
		text.font = Asset::Font::lowpoly;
		text.color = UI::default_color;
		text.anchor_x = UIText::Anchor::Center;
		text.anchor_y = UIText::Anchor::Center;
		text.size = 32.0f;

		if (manager.ref()->team.ref()->has_player())
			text.text(_(strings::victory));
		else
			text.text(_(strings::defeat));
		Vec2 pos = vp.size * Vec2(0.5f, 0.5f);
		UI::box(params, text.rect(pos).outset(16 * UI::scale), UI::background_color);
		text.draw(params, pos);
	}
}

s32 factorial(s32 x)
{
	s32 accum = 1;
	while (x > 1)
	{
		accum *= x;
		x--;
	}
	return accum;
}

s32 combination(s32 n, s32 choose)
{
	return factorial(n) / (factorial(n - choose) * factorial(choose));
}

PlayerCommon::PlayerCommon(PlayerManager* m)
	: cooldown(),
	username_text(),
	visibility_index(),
	manager(m)
{
	username_text.font = Asset::Font::lowpoly;
	username_text.size = 18.0f;
	username_text.anchor_x = UIText::Anchor::Center;
	username_text.text(m->username);
}

void PlayerCommon::awake()
{
	get<Health>()->hp_max = AWK_HEALTH;
	username_text.color = Team::ui_colors[(s32)get<AIAgent>()->team];
}

void PlayerCommon::update(const Update& u)
{
	if (has<Parkour>() || get<Transform>()->parent.ref())
	{
		// Either we are a Minion, or we're an Awk and we're attached to a surface
		// Either way, we need to decrement the cooldown timer
		cooldown = fmax(0.0f, cooldown - u.time.delta);
	}
}

void LocalPlayerControl::awk_bounce(const Vec3& new_velocity)
{
	Vec3 direction = Vec3::normalize(get<Awk>()->velocity);
	attach_quat = Quat::look(direction);
}

void LocalPlayerControl::awk_attached()
{
	Quat absolute_rot = get<Transform>()->absolute_rot();
	Vec3 wall_normal = absolute_rot * Vec3(0, 0, 1);
	Vec3 direction = Vec3::normalize(get<Awk>()->velocity);
	Vec3 up = Vec3::normalize(wall_normal.cross(direction));
	Vec3 right = direction.cross(up);
	if (right.dot(wall_normal) < 0.0f)
		right *= -1.0f;

	b8 vertical_surface = fabs(direction.y) < 0.99f;
	if (vertical_surface)
	{
		angle_horizontal = atan2f(direction.x, direction.z);
		angle_vertical = -asinf(direction.y);
	}

	// If we are spawning on to a flat floor, set attach_quat immediately
	// This preserves the camera rotation set by the PlayerSpawn
	if (!vertical_surface && direction.y == -1.0f)
		attach_quat = absolute_rot;
	else
		attach_quat = Quat::look(right);

	get<Audio>()->post_event(AK::EVENTS::STOP_FLY);
}

LocalPlayerControl::LocalPlayerControl(u8 gamepad)
	: angle_horizontal(),
	angle_vertical(),
	attach_quat(Quat::identity),
	gamepad(gamepad),
	fov_blend(),
	allow_zoom(true),
	try_jump(),
	try_parkour(),
	enable_input(),
	lean()
{
	camera = Camera::add();
	camera->fog = Game::data.mode == Game::Mode::Parkour;
}

LocalPlayerControl::~LocalPlayerControl()
{
	Audio::listener_disable(gamepad);
	camera->remove();
}

void LocalPlayerControl::awake()
{
	Audio::listener_enable(gamepad);

	if (has<Awk>())
	{
		link<&LocalPlayerControl::awk_attached>(get<Awk>()->attached);
		link_arg<Entity*, &LocalPlayerControl::hit_target>(get<Awk>()->hit);
		link_arg<const Vec3&, &LocalPlayerControl::awk_bounce>(get<Awk>()->bounce);
	}

	camera->team = (u8)get<AIAgent>()->team;
	camera->mask = 1 << camera->team;
}

#define MINION_CREDITS 10
#define AWK_CREDITS 50
void LocalPlayerControl::hit_target(Entity* target)
{
	player.ref()->msg(_(strings::target_hit));
	if (target->has<MinionAI>() && target->get<AIAgent>()->team != get<AIAgent>()->team)
		player.ref()->manager.ref()->add_credits(MINION_CREDITS);
	if (target->has<Awk>() && target->get<AIAgent>()->team != get<AIAgent>()->team)
		player.ref()->manager.ref()->add_credits(AWK_CREDITS);
}

b8 LocalPlayerControl::input_enabled() const
{
	return !Console::visible && enable_input;
}

void LocalPlayerControl::update_camera_input(const Update& u)
{
	const Settings& settings = Loader::settings();
	if (input_enabled())
	{
		if (gamepad == 0)
		{
			r32 s = LMath::lerpf(fov_blend, speed_mouse, speed_mouse_zoom);
			angle_horizontal -= s * (r32)u.input->cursor_x;
			angle_vertical += s * (r32)u.input->cursor_y;
		}

		if (u.input->gamepads[gamepad].active)
		{
			r32 s = LMath::lerpf(fov_blend, speed_joystick, speed_joystick_zoom);
			angle_horizontal -= s * u.time.delta * Input::dead_zone(u.input->gamepads[gamepad].right_x);
			angle_vertical += s * u.time.delta * Input::dead_zone(u.input->gamepads[gamepad].right_y);
		}

		angle_vertical = LMath::clampf(angle_vertical, PI * -0.495f, PI * 0.495f);
	}
}

Vec3 LocalPlayerControl::get_movement(const Update& u, const Quat& rot)
{
	Vec3 movement = Vec3::zero;
	if (input_enabled())
	{
		const Settings& settings = Loader::settings();

		if (u.input->get(settings.bindings.forward, gamepad))
			movement += Vec3(0, 0, 1);
		if (u.input->get(settings.bindings.backward, gamepad))
			movement += Vec3(0, 0, -1);
		if (u.input->get(settings.bindings.right, gamepad))
			movement += Vec3(-1, 0, 0);
		if (u.input->get(settings.bindings.left, gamepad))
			movement += Vec3(1, 0, 0);

		if (u.input->gamepads[gamepad].active)
		{
			movement += Vec3(-Input::dead_zone(u.input->gamepads[gamepad].left_x), 0, 0);
			movement += Vec3(0, 0, -Input::dead_zone(u.input->gamepads[gamepad].left_y));
		}

		movement = rot * movement;

		if (u.input->get(settings.bindings.up, gamepad))
			movement.y += 1;
		if (u.input->get(settings.bindings.down, gamepad))
			movement.y -= 1;
	}
	return movement;
}

void LocalPlayerControl::update(const Update& u)
{
	const Settings& settings = Loader::settings();

	{
		// Zoom
		r32 fov_blend_target = 0.0f;
		if (has<Awk>() && get<Transform>()->parent.ref() && Game::data.allow_detach && input_enabled())
		{
			if (u.input->get(settings.bindings.secondary, gamepad))
			{
				if (allow_zoom)
				{
					fov_blend_target = 1.0f;
					if (!u.last_input->get(settings.bindings.secondary, gamepad))
						get<Audio>()->post_event(AK::EVENTS::PLAY_ZOOM_IN);
				}
			}
			else
			{
				if (allow_zoom && u.last_input->get(settings.bindings.secondary, gamepad))
					get<Audio>()->post_event(AK::EVENTS::PLAY_ZOOM_OUT);
				allow_zoom = true;
			}
		}

		if (fov_blend < fov_blend_target)
			fov_blend = fmin(fov_blend + u.time.delta * zoom_speed, fov_blend_target);
		else if (fov_blend > fov_blend_target)
			fov_blend = fmax(fov_blend - u.time.delta * zoom_speed, fov_blend_target);
	}

	Vec3 camera_pos;
	Quat look_quat;

	if (has<Awk>())
	{
		// Awk control code
		if (get<Transform>()->parent.ref())
		{
			Quat rot = get<Transform>()->absolute_rot();

			r32 angle = Quat::angle(attach_quat, rot);
			if (angle > 0)
				attach_quat = Quat::slerp(fmin(1.0f, rotation_speed * u.time.delta), attach_quat, rot);

			// Look
			{
				Vec3 attach_normal = attach_quat * Vec3(0, 0, 1);

				update_camera_input(u);

				look_quat = Quat::euler(lean, angle_horizontal, angle_vertical);

				Vec3 forward = look_quat * Vec3(0, 0, 1);
				r32 dot = forward.dot(attach_normal);
				if (dot < -0.5f)
				{
					forward = Vec3::normalize(forward - (dot + 0.5f) * attach_normal);
					angle_horizontal = atan2f(forward.x, forward.z);
					angle_vertical = -asinf(forward.y);
					look_quat = Quat::euler(lean, angle_horizontal, angle_vertical);
				}
			}
		}
		else
		{
			Vec3 direction = Vec3::normalize(get<Awk>()->velocity);
			Quat rot = Quat::look(direction);
			r32 angle = Quat::angle(attach_quat, rot);
			if (angle > 0)
				attach_quat = Quat::slerp(fmin(1.0f, rotation_speed * u.time.delta), attach_quat, rot);
			look_quat = attach_quat;
		}

		camera_pos = get<Awk>()->center();

		// abilities
		for (s32 i = 0; i < ABILITY_COUNT; i++)
		{
			if (u.input->get(settings.bindings.abilities[i], gamepad) && !u.last_input->get(settings.bindings.abilities[i], gamepad))
				player.ref()->manager.ref()->abilities[i].use(entity());
		}
	}
	else
	{
		// Minion control code
		update_camera_input(u);
		look_quat = Quat::euler(lean, angle_horizontal, angle_vertical);
		{
			camera_pos = Vec3(0, 0, 0.05f);
			Quat q = Quat::identity;
			get<Parkour>()->head_to_object_space(&camera_pos, &q);
			camera_pos = get<Transform>()->to_world(camera_pos);
		}

		Vec3 movement = get_movement(u, Quat::euler(0, angle_horizontal, 0));
		Vec2 dir = Vec2(movement.x, movement.z);
		get<Walker>()->dir = dir;

		// parkour button
		b8 parkour_pressed = input_enabled() && u.input->get(settings.bindings.parkour, gamepad);

		if (get<Parkour>()->fsm.current == Parkour::State::WallRun && !parkour_pressed)
			get<Parkour>()->fsm.transition(Parkour::State::Normal);

		if (parkour_pressed && !u.last_input->get(settings.bindings.parkour, gamepad))
			try_parkour = true;
		else if (!parkour_pressed)
			try_parkour = false;

		if (try_parkour)
		{
			if (get<Parkour>()->try_parkour())
			{
				try_parkour = false;
				try_jump = false;
				try_slide = false;
			}
		}

		// jump button
		b8 jump_pressed = input_enabled() && u.input->get(settings.bindings.jump, gamepad);
		if (jump_pressed && !u.last_input->get(settings.bindings.jump, gamepad))
			try_jump = true;
		else if (!jump_pressed)
			try_jump = false;

		if (try_jump)
		{
			if (get<Parkour>()->try_jump(angle_horizontal))
			{
				try_parkour = false;
				try_jump = false;
				try_slide = false;
			}
		}
		
		// slide button
		b8 slide_pressed = input_enabled() && u.input->get(settings.bindings.slide, gamepad);

		if (get<Parkour>()->fsm.current == Parkour::State::Slide && !slide_pressed)
			get<Parkour>()->fsm.transition(Parkour::State::Normal);

		if (slide_pressed && !u.last_input->get(settings.bindings.slide, gamepad))
			try_slide = true;
		else if (!slide_pressed)
			try_slide = false;

		if (try_slide)
		{
			if (get<Parkour>()->try_slide())
			{
				try_parkour = false;
				try_jump = false;
				try_slide = false;
			}
		}

		{
			r32 arm_angle = LMath::clampf(angle_vertical * 0.5f, -PI * 0.15f, PI * 0.15f);
			get<Animator>()->override_bone(Asset::Bone::character_upper_arm_L, Vec3::zero, Quat::euler(arm_angle, 0, 0));
			get<Animator>()->override_bone(Asset::Bone::character_upper_arm_R, Vec3::zero, Quat::euler(-arm_angle, 0, 0));
		}

		r32 lean_target = 0.0f;

		if (get<Parkour>()->fsm.current == Parkour::State::WallRun)
		{
			Parkour::WallRunState state = get<Parkour>()->wall_run_state;
			const r32 wall_run_lean = 10.0f * PI / 180.0f;
			if (state == Parkour::WallRunState::Left)
				lean_target = -wall_run_lean;
			else if (state == Parkour::WallRunState::Right)
				lean_target = wall_run_lean;

			Vec3 wall_normal = get<Parkour>()->last_support.ref()->get<Transform>()->to_world_normal(get<Parkour>()->relative_wall_run_normal);

			Vec3 forward = look_quat * Vec3(0, 0, 1);

			if (get<Parkour>()->wall_run_state == Parkour::WallRunState::Forward)
				wall_normal *= -1.0f; // Make sure we're always facing the wall
			else
			{
				// We're running along the wall
				// Make sure we can't look backward
				clamp_rotation(Quat::euler(0, get<Walker>()->target_rotation, 0) * Vec3(0, 0, 1));
			}

			clamp_rotation(wall_normal);
		}
		else if (get<Parkour>()->fsm.current == Parkour::State::Slide)
		{
			lean_target = (PI / 180.0f) * 15.0f;
			clamp_rotation(Quat::euler(0, get<Walker>()->target_rotation, 0) * Vec3(0, 0, 1));
		}
		else
		{
			lean_target = get<Walker>()->net_speed * LMath::angle_to(angle_horizontal, last_angle_horizontal) * (1.0f / 180.0f) / u.time.delta;

			get<Walker>()->target_rotation = angle_horizontal;

			// make sure our body is facing within 90 degrees of our target rotation
			r32 delta = LMath::angle_to(get<Walker>()->rotation, angle_horizontal);
			if (delta > PI * 0.5f)
				get<Walker>()->rotation = LMath::angle_range(get<Walker>()->rotation + delta - PI * 0.5f);
			else if (delta < PI * -0.5f)
				get<Walker>()->rotation = LMath::angle_range(get<Walker>()->rotation + delta + PI * 0.5f);
		}

		lean += (lean_target - lean) * fmin(1.0f, 15.0f * u.time.delta);
		look_quat = Quat::euler(lean, angle_horizontal, angle_vertical);
	}
	
	s32 player_count = LocalPlayer::list.count();
	Camera::ViewportBlueprint* viewports = Camera::viewport_blueprints[player_count - 1];
	Camera::ViewportBlueprint* blueprint = &viewports[gamepad];

	if (has<Awk>())
		camera->range = AWK_MAX_DISTANCE;
	else
		camera->range = 0.0f;

	camera->viewport =
	{
		Vec2((s32)(blueprint->x * (r32)u.input->width), (s32)(blueprint->y * (r32)u.input->height)),
		Vec2((s32)(blueprint->w * (r32)u.input->width), (s32)(blueprint->h * (r32)u.input->height)),
	};
	r32 aspect = camera->viewport.size.y == 0 ? 1 : (r32)camera->viewport.size.x / (r32)camera->viewport.size.y;
	camera->perspective(LMath::lerpf(fov_blend, fov_initial, fov_zoom), aspect, 0.02f, Skybox::far_plane);

	// Camera matrix
	camera->pos = camera_pos + (Game::data.third_person ? look_quat * Vec3(0, 0, -2) : Vec3::zero);
	camera->rot = look_quat;
	if (has<Awk>())
		camera->wall_normal = look_quat.inverse() * ((get<Transform>()->absolute_rot() * get<Awk>()->lerped_rotation) * Vec3(0, 0, 1));
	else
		camera->wall_normal = Vec3(0, 0, 1);

	tracer.type = TraceType::None;
	if (has<Awk>() && get<Transform>()->parent.ref() && get<PlayerCommon>()->cooldown == 0.0f)
	{
		// Display trajectory
		Vec3 trace_dir = look_quat * Vec3(0, 0, 1);
		// Make sure we're not shooting into the wall we're on.
		// If we're a sentinel, no need to worry about that.
		if (has<Parkour>() || trace_dir.dot(get<Transform>()->absolute_rot() * Vec3(0, 0, 1)) > 0.0f)
		{
			Vec3 trace_start = camera_pos;
			Vec3 trace_end = trace_start + trace_dir * AWK_MAX_DISTANCE;

			AwkRaycastCallback rayCallback(trace_start, trace_end, entity());
			rayCallback.m_flags = btTriangleRaycastCallback::EFlags::kF_FilterBackfaces
				| btTriangleRaycastCallback::EFlags::kF_KeepUnflippedNormal;
			rayCallback.m_collisionFilterMask = rayCallback.m_collisionFilterGroup = btBroadphaseProxy::AllFilter;

			Physics::btWorld->rayTest(trace_start, trace_end, rayCallback);

			if (rayCallback.hasHit())
			{
				tracer.type = rayCallback.hit_target ? TraceType::Target : TraceType::Normal;
				tracer.pos = rayCallback.m_hitPointWorld;

				short group = rayCallback.m_collisionObject->getBroadphaseHandle()->m_collisionFilterGroup;
				if (group & (CollisionWalker | CollisionInaccessible))
					tracer.type = TraceType::None; // We can't go there
			}
		}

		if (tracer.type != TraceType::None && Game::data.allow_detach && input_enabled())
		{
			b8 go = u.input->get(settings.bindings.primary, gamepad) && !u.last_input->get(settings.bindings.primary, gamepad);

			if (go && get<Awk>()->detach(u, look_quat * Vec3(0, 0, 1)))
			{
				allow_zoom = false;
				attach_quat = look_quat;
				get<Audio>()->post_event(AK::EVENTS::PLAY_FLY);
			}
		}
	}

	Audio::listener_update(gamepad, camera_pos, look_quat);

	last_angle_horizontal = angle_horizontal;
}

void LocalPlayerControl::clamp_rotation(const Vec3& direction)
{
	Quat look_quat = Quat::euler(lean, angle_horizontal, angle_vertical);
	Vec3 forward = look_quat * Vec3(0, 0, 1);

	r32 dot = forward.dot(direction);
	if (dot < 0.0f)
	{
		forward = Vec3::normalize(forward - dot * direction);
		angle_horizontal = atan2f(forward.x, forward.z);
		angle_vertical = -asinf(forward.y);
	}
}

LocalPlayerControl* LocalPlayerControl::player_for_camera(const Camera* cam)
{
	for (auto i = LocalPlayerControl::list.iterator(); !i.is_last(); i.next())
	{
		if (i.item()->camera == cam)
			return i.item();
	}
	return nullptr;
}

void LocalPlayerControl::draw_alpha(const RenderParams& params) const
{
	if (params.technique != RenderTechnique::Default)
		return;

	if (params.camera == camera)
	{
		AI::Team team = get<AIAgent>()->team;

		const Rect2& viewport = params.camera->viewport;

		// reticle
		if (has<Awk>() && get<Transform>()->parent.ref())
		{
			r32 cooldown = get<PlayerCommon>()->cooldown;
			r32 radius = cooldown == 0.0f ? 0.0f : fmax(0.0f, 32.0f * (get<PlayerCommon>()->cooldown / AWK_MAX_DISTANCE_COOLDOWN));
			if (radius > 0 || tracer.type == TraceType::None || !Game::data.allow_detach)
			{
				// hollow reticle
				UI::centered_border(params, { viewport.size * Vec2(0.5f, 0.5f), Vec2(5.0f + radius) * UI::scale }, 2, UI::accent_color, PI * 0.25f);
			}
			else
			{
				// solid reticle
				Vec2 a;
				if (UI::project(params, tracer.pos, &a))
				{
					if (tracer.type == TraceType::Target)
					{
						UI::centered_box(params, { a, Vec2(7) * UI::scale }, UI::alert_color, PI * 0.25f);

						UI::centered_box(params, { a + Vec2(12.0f, 12.0f) * UI::scale, Vec2(8.0f, 2.0f) * UI::scale }, UI::accent_color, PI * 0.25f);
						UI::centered_box(params, { a + Vec2(-12.0f, -12.0f) * UI::scale, Vec2(8.0f, 2.0f) * UI::scale }, UI::accent_color, PI * 0.25f);
						UI::centered_box(params, { a + Vec2(-12.0f, 12.0f) * UI::scale, Vec2(2.0f, 8.0f) * UI::scale }, UI::accent_color, PI * 0.25f);
						UI::centered_box(params, { a + Vec2(12.0f, -12.0f) * UI::scale, Vec2(2.0f, 8.0f) * UI::scale }, UI::accent_color, PI * 0.25f);
					}
					else
						UI::centered_box(params, { a, Vec2(6) * UI::scale }, UI::accent_color, PI * 0.25f);
				}
			}
		}

		// compass
		Vec2 compass_size = Vec2(fmin(viewport.size.x, viewport.size.y) * 0.3f);
		UI::mesh(params, Asset::Mesh::compass_inner, viewport.size * Vec2(0.5f, 0.5f), compass_size, UI::accent_color, angle_vertical);
		UI::mesh(params, Asset::Mesh::compass_outer, viewport.size * Vec2(0.5f, 0.5f), compass_size, UI::accent_color, -angle_horizontal);

		// health indicator
		{
			const Health* health = get<Health>();
			const Vec2 box_size = Vec2(16.0f) * UI::scale;
			const r32 box_spacing = 8.0f * UI::scale;
			const Vec4& color = Team::ui_colors[(s32)team];

			UIText text;
			text.font = Asset::Font::lowpoly;
			text.anchor_x = UIText::Anchor::Min;
			text.anchor_y = UIText::Anchor::Min;
			text.size = 16.0f;
			text.color = UI::default_color;
			text.text(_(strings::hp));

			r32 total_width = (text.bounds().x + box_spacing + health->hp_max * (box_size.x + box_spacing)) - box_spacing;

			Vec2 pos = viewport.size * Vec2(0.5f, 0.1f) + Vec2(total_width * -0.5f, 0.0f);

			UI::box(params, Rect2(pos, Vec2(total_width, box_size.y)).outset(8 * UI::scale), UI::background_color);

			text.draw(params, pos);
			
			pos.x += text.bounds().x + box_spacing;

			for (s32 i = 0; i < health->hp_max; i++)
			{
				UI::border(params, { pos, box_size }, 2, color);
				UI::box(params, { pos, box_size }, i < health->hp ? color : UI::background_color);
				pos.x += box_size.x + box_spacing;
			}
		}

		if (has<Awk>())
		{
			// abilities
			{
				const Settings& settings = Loader::settings();
				UIText text;
				text.font = Asset::Font::lowpoly;
				text.anchor_x = UIText::Anchor::Min;
				text.anchor_y = UIText::Anchor::Center;
				text.size = 16.0f;
				Vec2 icon_pos = viewport.size * Vec2(0.1f, 0.12f) + Vec2(10.0f * UI::scale, 44.0f * UI::scale);
				for (s32 i = ABILITY_COUNT - 1; i >= 0; i--)
				{
					const AbilitySlot& slot = player.ref()->manager.ref()->abilities[i];

					if (slot.ability != Ability::None)
					{
						const AbilitySlot::Info& info = AbilitySlot::info[(s32)slot.ability];
						text.text(_(strings::binding), settings.bindings.abilities[i].string(params.sync->input.gamepads[gamepad].active));

						r32 text_offset = 24.0f * UI::scale;
						r32 icon_size = 8.0f * UI::scale;
						Vec2 text_pos = icon_pos + Vec2(text_offset, 0);

						Rect2 box = text.rect(text_pos).outset(8.0f * UI::scale);
						box.pos.x -= (text_offset + icon_size);
						box.size.x += (text_offset + icon_size);
						UI::box(params, box, UI::background_color);

						if (slot.cooldown > 0.0f)
						{
							r32 cooldown_normalized = slot.cooldown / info.cooldown;
							UI::box(params, { box.pos, Vec2(box.size.x * cooldown_normalized, box.size.y) }, UI::subtle_color);
							text.color = UI::disabled_color;
						}
						else
							text.color = UI::default_color;

						if (info.icon != AssetNull)
							UI::mesh(params, info.icon, icon_pos, Vec2(UI::scale * 16.0f), text.color);

						text.draw(params, text_pos);

						icon_pos.y += box.size.y;
					}
				}
			}

			// stealth indicator
			{
				r32 stealth_timer = get<Awk>()->stealth_timer;
				if (stealth_timer > 0.0f)
				{
					UIText text;
					text.text("%s %d", _(strings::stealth), (s32)stealth_timer + 1);
					text.font = Asset::Font::lowpoly;
					text.anchor_x = UIText::Anchor::Center;
					text.anchor_y = UIText::Anchor::Center;
					text.size = 16.0f;
					Vec2 pos = viewport.size * Vec2(0.5f, 0.65f);
					UI::box(params, text.rect(pos).outset(8.0f * UI::scale), UI::background_color);
					text.draw(params, pos);
				}
			}
		}
	}
}

}