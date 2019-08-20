#include <linmath.h>

#include "game.h"

#include "audio.h"
#include "debug.h"
#include "render.h"
#include "input.h"
#include "conf.h"

#include <time.h>

static a_buf buffer;
static a_music* music;

static r_shader shader;
static r_sheet sheet;
static r_anim anim;

static int dir_x[16];
static int dir_y[16];

static int buttons[MAX_JOY_BUTTONS];
static float axes[12];

static int ui_active = 0;
static int ui_state = 0;

int g_init(){
	sheet  = r_get_sheet("res/tex/test_sheet.png", 16, 16);
	shader = r_get_shader("res/shd/main.v", "res/shd/main.f");
	r_map_shader(shader, "default");
	unsigned int frames[5] = { 4, 5, 6, 7, 4};
	unsigned int frames2[5] = { 0, 1, 2, 3, 1};

	time_t t;
	srand((unsigned) time(&t));

	anim = r_get_anim(sheet, frames, 5, 24);
	r_anim anim2 = r_get_anim(sheet, frames2, 5, 24);

	r_cache_anim(anim, "test");
	r_cache_anim(anim2, "test2");

	r_anim* _anim = r_get_anim_n("test");

	//buffer = a_create_buffer("res/snd/test.ogg");

	int data_len;
	unsigned char* data = c_get_file_contents("res/snd/test.ogg", &data_len);

	if(!data){
		_l("No data loaded for music file.\n");
	}

	//buffer = a_get_buf(data, data_len);
	music = a_create_music(data, data_len, NULL);
	a_play_music(music);

	for(int i=0;i<16;++i){
		dir_x[i] = (rand() % 3) - 1;
		dir_y[1] = (rand() % 3) - 1;
	}

	float offset_x = 100.f;
	float offset_y = 100.f;
	for(int i=0;i<16;++i){
		int x = i % 4;
		int y = i / 4;
		r_drawable* d = r_get_drawable(_anim, shader, (vec2){16.f, 16.f}, (vec2){(32.f * x) + offset_x, (32.f * y) + offset_y});
		if(!dir_x[i]){
			d->flip_x = 1;	
		}

		if(dir_y[i]){
			d->flip_y = 1;
		}
		r_anim_p(&d->anim);
	}

	i_create_joy(0);
	int type = i_get_joy_type(0);
	if(type == XBOX_360_PAD){
		printf("XBOX FOUND!\n");
	}

	_l("Initialized game.\n");
	return 1;	
}

void g_exit(){

}

void g_input(long delta){
	if(ui_active){

		
	}else{
		//i_get_joy_buttons(buttons, 12);
		if(i_key_clicked('P')){
			a_play_sfx(&buffer, NULL); 
		}

		if(i_key_clicked('R')){
			int r = (rand() % 15) + 1;//non-zero uid
			r_drawable* d = r_get_drawablei(r);
			r_anim* anim = r_get_anim_n("test2");
			r_drawable_set_anim(d, anim);
		}

		if(i_joy_button_down(0)){
			_l("Down!\n");
		}

		float change_x = 0.f; 
		float change_y = 0.f;

		float _x = i_joy_axis_delta(XBOX_L_X);
		float _y = i_joy_axis_delta(XBOX_L_Y);
		float x_axis = i_joy_axis(XBOX_L_X);
		float y_axis = -1.f * i_joy_axis(XBOX_L_Y);

		if(_x != 0 || x_axis >= 0.75f || x_axis <= -0.75f){
			_x = x_axis;
		}

		if(_y != 0 || y_axis >= 0.75f || y_axis <= -0.75f){
			_y = y_axis;
		}

		if(i_key_down('D')){
			change_x += delta;
		}else if(i_key_down('A')){
			change_x -= delta;
		}else if(_x > 0.f){
			change_x += delta * _x;
		}else if(_x < 0.f){
			change_x += delta * _x;
		}

		if(i_key_down('W')){
			change_y += delta;
		}else if(i_key_down('S')){
			change_y -= delta;
		}else if(_y > 0) {
			change_y += delta * _y;
		}else if(_y < 0){
			change_y += delta * _y;
		}

		if(change_x != 0 || change_y != 0){
			r_move_cam(change_x, change_y);
		}
	}

	if(i_key_clicked(GLFW_KEY_ESCAPE)){
		r_request_close();	
	}

}

void g_update(long delta){
	a_update(delta);
	r_update(delta);
	r_update_batch(shader, &sheet);
}

void g_render(long delta){
	/*
	 *struct nk_ctx* ctx = g_window.ui;
	if (nk_begin(ctx, "Demo", nk_rect(0, 0, g_window.width, g_window.height), NK_WINDOW_BORDER)){
		enum {EASY, HARD};
		static int op = EASY;
		static int property = 20;
		nk_layout_row_dynamic(ctx, 30, 4);
		if (nk_button_label(ctx, "button"))
			fprintf(stdout, "button pressed\n");
		if (nk_button_label(ctx, "button"))
			fprintf(stdout, "button pressed\n");
		if (nk_button_label(ctx, "button"))
			fprintf(stdout, "button pressed\n");
		if (nk_button_label(ctx, "button"))
			fprintf(stdout, "button pressed\n");


		nk_layout_row_dynamic(ctx, 30, 4);
		if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
		if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;

		nk_layout_row_dynamic(ctx, 25, 4);
		nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);
	}
	nk_end(ctx);


	 */

	int width, height;
	r_window_get_size(&width, &height);	

	int ui_width = 720;
	int ui_height = 360;

	int offset_x = (width - ui_width) / 2;
	int offset_y = (height - ui_height) / 2;

	if(r_ui_window(offset_x, offset_y, 720, 360)){
		//add spacing of 15px
		r_ui_row(15, 1);
		r_ui_row(35, 5);
		r_ui_spacing(1);
		if(r_ui_button("Test Button")) _l("Test button pressed.\n");
		r_ui_spacing(1);
		if(r_ui_button("Even Testier.")) _l("Testier!\n");

		r_ui_row(35, 2);
		r_ui_row(30, 4);
		static int op = 1;
		if(r_ui_option("One", (op == 1))) op = 1;
		r_ui_spacing(1);
		if(r_ui_option("Two", (op == 2))) op = 2;		

		r_ui_row(25, 3);
		r_ui_spacing(1);
		
		static int _prog = 5;
		static float _slide = 10;
		
		//TODO progress bar styling
		//r_ui_progress(&_prog, 100, 0);		
			
		if(r_ui_slider(0.f, &_slide, 100.f, 1.f)) _l("Slide Value: %f\n", _slide);
		r_ui_row(25, 4);
		if(r_ui_radio("Test", &_prog)) _l("Radio button");
		if(r_ui_checkbox("Test check.", &_prog)) _l("Check box");
	}
	r_ui_end();

	for(int i=1;i<17;++i){
		r_drawable* d = r_get_drawablei(i);
		d->position[0] += dir_x[i] * delta * 0.05f;
		d->position[1] += dir_y[i] * delta * 0.05f;
		d->change = 1;		
	}

	r_draw_call(shader, &sheet);

	r_update_ui();
	
	r_draw_ui();
}
