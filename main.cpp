#include <graphics.h>
#include <string>
#include <vector>

#define PI 3.141592653589793
#define  KEY_W 0x57
#define  KEY_S 0x53
#define  KEY_A 0x41
#define  KEY_D 0x44 
#define  frame_time 1000.0/144.0
#define  ENEMY_ANIM_NUM 4
#define  PLAYER_ANIM_NUM 2

bool running = true;
bool is_game_started = false;

int idx_current_anim = 0;

const int WINDOW_HEIGHT=720;
const int WINDOW_WIDTH=1280;
const int BUTTOM_WIDTH = 250;
const int BUTTOM_HEIGHT = 120;

IMAGE img_background;
IMAGE img_ky;
IMAGE img_menu;

#pragma comment(lib,"MSIMG32.LIB")
#pragma comment(lib,"Winmm.lib")

inline void putimage_alpha(int x, int y, IMAGE* img)
{
	int w = img->getwidth();
	int h = img->getheight();
	AlphaBlend(GetImageHDC(NULL), x, y, w, h,
		GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER,0,255,AC_SRC_ALPHA });
}
class Atlas {
public:
	Atlas(LPCTSTR path ,int num)
	{
		TCHAR path_file[256];
		for (size_t i = 0; i < num; i++)
		{
			_stprintf_s(path_file, path, i);

			IMAGE* frame = new IMAGE();
			loadimage(frame, path_file);
			frame_list.push_back(frame);
		}
	}
	~Atlas()
	{
		for (size_t i = 0; i < frame_list.size(); i++)
		{
			delete frame_list[i];
		}
	}
public:
	std::vector<IMAGE*> frame_list;//帧列表
 }; //图集类

Atlas* atlas_player_right;
Atlas* atlas_player_left;
Atlas* atlas_enemy_right;
Atlas* atlas_enemy_left;


class Animation
{
public:
	//加载图片
	Animation(Atlas* atlas,int interval) //图片路径，帧数量，帧间隔
	{
		interval_ms = interval;
		anim_atlas = atlas;
	}
	~Animation() = default;
	void Play(int x, int y, int delta) //播放动画
	{
		timer += delta;
		if (timer >= interval_ms)
		{
			idx_frame = (idx_frame + 1) % anim_atlas->frame_list.size();
			timer = 0;
		}
		putimage_alpha(x, y, anim_atlas->frame_list[idx_frame]);
	}
private:
	int timer = 0; //动画计时器
	int idx_frame = 0; //动画帧索引
	int interval_ms = 0;
private:
	Atlas* anim_atlas;
};
class Player
{
public:	
	const int WIDTH = 100;
	const int HEIGHT = 100;
public :
	Player()
	{
		 anim_left=new Animation(atlas_player_left, 45);
		 anim_right=new Animation(atlas_player_right, 45);
	}
	~Player()
	{
		delete anim_left, anim_right;
	}
	void ProcessEvent(const ExMessage &msg)
	{
		// 只能读取msg中的数据，不能修改
		switch (msg.message)
		{
		case WM_KEYDOWN:
			switch (msg.vkcode)
			{
			case KEY_W:
				key_w_down = true;
				break;
			case KEY_S:
				key_s_down = true;
				break;
			case KEY_A:
				key_a_down = true;
				break;
			case KEY_D:
				key_d_down = true;
				break;
			}
			break;
		case WM_KEYUP:
			switch (msg.vkcode)
			{
			case KEY_W:
				key_w_down = false;
				break;
			case KEY_S:
				key_s_down = false;
				break;
			case KEY_A:
				key_a_down = false;
				break;
			case KEY_D:
				key_d_down = false;
				break;
			}
			break;
		}
	}

	void Move()
	{
		 dir_x = key_a_down - key_d_down;
		int dir_y = key_w_down - key_s_down;
		double dir_len = sqrt(pow(dir_x, 2) + pow(dir_y, 2));
		if (dir_len != 0)
		{
			double normalized_x= dir_x / dir_len;
			double normalized_y= dir_y / dir_len;
			position.x -= (int)(SPEED * normalized_x);
			position.y -= (int)(SPEED * normalized_y);
		}
		if (position.x < 0) position.x = 0;
		if (position.y < 0) position.y = 0;
		if (position.x + WIDTH > WINDOW_WIDTH) position.x = WINDOW_WIDTH - WIDTH;
		if (position.y + HEIGHT > WINDOW_HEIGHT) position.y = WINDOW_HEIGHT - HEIGHT;
	}
	void Draw(int delta)
	{
		static bool facing_left = false;
		if (dir_x > 0)
			facing_left = true;
		else if (dir_x < 0)
			facing_left = false;
		if (facing_left)
			anim_left->Play(position.x, position.y, delta);
		else
			anim_right->Play(position.x, position.y, delta);
	}
	const POINT& GetPlayerPosition() const
	{
		return  position;
	}
private:
	const int SPEED = 5;
private:
	Animation* anim_left, * anim_right;
	POINT position = { 640,360 };
	bool key_w_down = false;
	bool key_a_down = false;
	bool key_s_down = false;
	bool key_d_down = false;
	int dir_x=0;
};
class Bullets {
public:
	POINT position = { 0,0 };
public:
	Bullets() = default;
	~Bullets() = default;
	void Draw() const
	{
		setlinecolor(RGB(255, 155, 50));
		setfillcolor(RGB(200, 75, 10));
		fillcircle(position.x, position.y, radius);
	}
private:
	const int radius = 10; //子弹半径
};
class Enemy
{
public:
	Enemy()
	{
		anim_left_1 = new Animation(atlas_enemy_left, 60);
		anim_right_1 = new Animation(atlas_enemy_right, 60);

		enum class SpawnEdge
		{
			Up = 0,
			Down,
			Left,
			Right
		};
		//将敌人放置在地图外边界处随机位置
		SpawnEdge edge = (SpawnEdge)(rand() % 4);
		switch (edge)
		{
		case SpawnEdge::Up:
			position.x = rand() % WINDOW_WIDTH;
			position.y = -HEIGHT;
			break;
		case SpawnEdge::Down:
			position.x = rand() % WINDOW_WIDTH;
			position.y = WINDOW_HEIGHT;
			break;
		case SpawnEdge::Left:
			position.x = -WIDTH;
			position.y = rand() % WINDOW_HEIGHT;
			break;
		case SpawnEdge::Right:
			position.x = WINDOW_WIDTH;
			position.y = rand() % WINDOW_HEIGHT;
			break;
		default:
			break;
		}
	}
	bool CheckBulletsCollision( Bullets*  bullets)
	{
		bool if_bullets_hit_enemy_x = bullets->position.x >= position.x && bullets->position.x <= (position.x + WIDTH);
		bool if_bullets_hit_enemy_y = bullets->position.y >= position.y && bullets->position.y <= (position.y + HEIGHT);
		return if_bullets_hit_enemy_x && if_bullets_hit_enemy_y;
	}
	bool CheckPlayerCollision(const Player& player)
	{
		POINT  enemy_pos_center = { position.x + WIDTH / 2,position.y + HEIGHT / 2 };
		bool if_enemy_hit_player_x = enemy_pos_center.x >= player.GetPlayerPosition().x && enemy_pos_center.x <= (player.GetPlayerPosition().x + player.WIDTH);
		bool if_enemy_hit_player_y = enemy_pos_center.y >= player.GetPlayerPosition().y && enemy_pos_center.y <= (player.GetPlayerPosition().y + player.HEIGHT);
		return if_enemy_hit_player_x && if_enemy_hit_player_y;
	}
	void GetHurt()
	{
		alive = false;
	}
	bool CheckAlive()
	{
		return alive;
	}
	void Move(const Player& player)
	{
		const POINT& player_position = player.GetPlayerPosition();
		int dir_x = player_position.x - position.x;
		int dir_y = player_position.y - position.y;
		double dir_len = sqrt(pow(dir_x, 2) + pow(dir_y, 2));
		if (dir_len != 0)
		{
			double normalized_x = dir_x / dir_len;
			double normalized_y = dir_y / dir_len;
			position.x += (int)(SPEED * normalized_x);
			position.y += (int)(SPEED * normalized_y);
		}
		if (dir_x < 0)
			facing_left = true;
		else if (dir_x > 0)
			facing_left = false;
	}
	void Draw(int delta)
	{
		if (facing_left)
			anim_left_1->Play(position.x, position.y, delta);
		else
			anim_right_1->Play(position.x, position.y, delta);
		//if (facing_left)
		//	anim_left_2->Play(position.x, position.y, delta);
		//else
		//	anim_right_2->Play(position.x, position.y, delta);
	}
	~Enemy()
	{
		delete anim_left_1, anim_right_1;
		//delete anim_left_2, anim_right_2;

	}

private:
	const int SPEED = 2;
	const int WIDTH = 100;
	const int HEIGHT = 100;

private:
	Animation* anim_left_1, * anim_right_1;
	//Animation* anim_left_2, * anim_right_2;
	POINT position = { 0,0 };
	bool facing_left = false;
	bool alive = true;
};
class Button {
public:
	Button(RECT rect,LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
	{
		region = rect;

		loadimage(&img_idle, path_img_idle);
		loadimage(&img_hovered, path_img_hovered);
		loadimage(&img_pushed, path_img_pushed);

	}
	~Button() = default;
	void ProgressEvent(const ExMessage& msg)
	{
		switch (msg.message)
		{
		case WM_MOUSEMOVE:
			if (status == Status::Idle && CheckCursorHit(msg.x, msg.y))
				status = Status::Hovered;
			else if (status == Status::Hovered && !CheckCursorHit(msg.x, msg.y))
				status = Status::Idle;
			break;
		case WM_LBUTTONDOWN:
			if (CheckCursorHit(msg.x, msg.y))
				status = Status::Pushed;
			break;
		case WM_LBUTTONUP:
			if (status == Status::Pushed)
				Onclick(); //虚函数，用来处理不同逻辑的点击
			break;
		default:
			break;
		}
	}
	void Draw()
	{
		switch (status)
		{
		case Status::Idle:
			putimage(region.left, region.top, &img_idle);
			break;
		case Status::Hovered:
			putimage(region.left, region.top, &img_hovered);
			break;
		case Status::Pushed:
			putimage(region.left, region.top, &img_pushed);
			break;
		}
	}
protected :
	virtual void Onclick() = 0;
private:
	enum class Status
	{
		Idle = 0,
		Hovered,
		Pushed
	};
private:
	RECT region;
	IMAGE img_idle;
	IMAGE img_hovered;
	IMAGE img_pushed;
	Status status = Status::Idle;
private:
	bool CheckCursorHit(int x,int y)
	{
		return x >= region.left && x <= region.right && y >= region.top && y <= region.bottom;
	}
};

class StartGameButton :public Button
{
public :
	StartGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		:Button( rect,  path_img_idle,  path_img_hovered,  path_img_pushed){}
	~StartGameButton() = default;
protected :
	void Onclick()
	{
		is_game_started = true;
		mciSendString(_T("stop menu"), NULL, 0, NULL);
		mciSendString(_T("close menu"), NULL, 0, NULL);
		mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);

	}
};
class QuitGameButton :public Button
{
public:
	QuitGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		:Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {}
	~QuitGameButton() = default;
protected:
	void Onclick()
	{
		running = false;
	}
};
//===================================================
//生成新的敌人
void TryGenerateEnemy(std::vector < Enemy*>& enemy_list)
{
	const int INTERVAL = 100;
	static int counter = 0;
	if ((++counter) % INTERVAL == 0)
		enemy_list.push_back(new Enemy());
}
//生成子弹
void UpdateBullets(std::vector < Bullets*>& bullets_list,const Player &player, int score)
{
	const double Radial_Speed = 0.0045; //径向速度
	const double Tangent_Speed = 0.0055; //切向速度
	double radian_interval = 2 * PI / bullets_list.size(); //子弹间弧度间隔
	POINT player_position = player.GetPlayerPosition();
	double radius = 100 + 25 * sin(GetTickCount() * Radial_Speed);//半径
	static int idx_count = 0;
	// 限制子弹数量并避免重复添加
	static int last_score = 0;
	if (bullets_list.size() <= 100)
	{
		if (score % 5 == 0 && score != 0 && score != last_score)
		{
			bullets_list.push_back(new Bullets());
			last_score = score;
		}
	}
	for (size_t i = 0; i < bullets_list.size(); i++)
	{
		double radian = GetTickCount() * Tangent_Speed + radian_interval * i;//当前子弹弧度值
		bullets_list[i]->position.x = player_position.x + player.WIDTH / 2 + (int)(radius * sin(radian));
		bullets_list[i]->position.y = player_position.y + player.HEIGHT / 2 + (int)(radius * cos(radian));
	}
}
void DrawPlayerScore(int score)
{
	static TCHAR text[64],text2[64];
	_stprintf_s(text, _T("当前玩家得分:%d"),score);//把该字符串输入对应字符数组
	_stprintf_s(text2, _T("WASD控制上下左右"));
	setbkmode(TRANSPARENT);
	settextcolor(RGB(255, 85, 185));
	outtextxy(10, 10, text);
	outtextxy(10, 30, text2);

}
int main()
{
	initgraph(WINDOW_WIDTH, WINDOW_HEIGHT);
	//图集资源
	
	atlas_enemy_left = new Atlas(_T("img\\xiaobao\\enemy_left_%d.png"),ENEMY_ANIM_NUM);
	atlas_enemy_right = new Atlas(_T("img\\xiaobao\\enemy_right_%d.png"), ENEMY_ANIM_NUM);
	atlas_player_left = new Atlas(_T("img\\leilei\\player_left_%d.png"), PLAYER_ANIM_NUM);
	atlas_player_right = new Atlas(_T("img\\leilei\\player_right_%d.png"), PLAYER_ANIM_NUM);
	//音频资源
	mciSendString(_T("open msc/menu.mp3 alias menu"), NULL, 0, NULL);
	mciSendString(_T("open msc/bgm.mp3 alias bgm"), NULL, 0, NULL);
	mciSendString(_T("open msc/bullets.wav alias bullets"), NULL, 0, NULL);

	std::vector<Enemy*> enemy_list_2;
	std::vector<Enemy*> enemy_list;
	std::vector<Bullets*> bullets_list = { new Bullets() };

	ExMessage msg;
	Player player;
	static int score=0;

	RECT region_btm_start_game, region_btm_quit_game;
	region_btm_start_game.left =320-125 ;
	region_btm_start_game.right = region_btm_start_game.left+BUTTOM_WIDTH;
	region_btm_start_game.top = 500;
	region_btm_start_game.bottom = region_btm_start_game.top+BUTTOM_HEIGHT;
	region_btm_quit_game.left = 960-125;
	region_btm_quit_game.right = region_btm_quit_game.left + BUTTOM_WIDTH;
	region_btm_quit_game.top = 500;
	region_btm_quit_game.bottom = region_btm_quit_game.top+BUTTOM_HEIGHT;


	StartGameButton start_game_btm = StartGameButton(region_btm_start_game, _T("img\\buttom\\start_idle.png"), _T("img\\buttom\\start_hovered.png"), _T("img\\buttom\\start_pushed.png"));
	QuitGameButton quit_game_btm = QuitGameButton(region_btm_quit_game, _T("img\\buttom\\exit_idle.png"), _T("img\\buttom\\exit_hovered.png"), _T("img\\buttom\\exit_pushed.png"));
	
	loadimage(&img_menu, _T("img\\menu.jpg"));
	loadimage(&img_background, _T("img\\background.png"));
	mciSendString(_T("play menu repeat from 0"), NULL, 0, NULL);
	BeginBatchDraw();
	while (running)
	{
		DWORD start_time = GetTickCount();
		while (peekmessage(&msg))
		{
			if (is_game_started)
			{
				player.ProcessEvent(msg);
			}
			else
			{
				start_game_btm.ProgressEvent(msg);
				quit_game_btm.ProgressEvent(msg);

			}
		}
		if (is_game_started)
		{
			UpdateBullets(bullets_list, player, score);
			player.Move();
			TryGenerateEnemy(enemy_list);
			for (Enemy* enemy : enemy_list)
			{
				enemy->Move(player);
				if (enemy->CheckPlayerCollision(player))
				{
					static TCHAR text[128];
					_stprintf_s(text, _T("最终得分:%d"), score);
					MessageBox(GetHWnd(), text, _T("游戏结束"), MB_OK);
					running = false;
					break;
				}
				for ( Bullets*  bullets : bullets_list)
				{
					if (enemy->CheckBulletsCollision(bullets))
					{
						score++;
						enemy->GetHurt();
						mciSendString(_T("play bullets from 0"), NULL, 0, NULL);
						break;
					}
				}
			}
			for (size_t i = 0; i < enemy_list.size(); i++)
			{
				Enemy* enemy = enemy_list[i];
				if (!enemy->CheckAlive()) {
					std::swap(enemy_list[i], enemy_list.back());
					enemy_list.pop_back();
					delete enemy;
				}
			}
			
		}
		
		cleardevice();
		if (is_game_started)
		{
			putimage(0, 0, &img_background);
			player.Draw(frame_time);
			for (Enemy* enemy : enemy_list)
				enemy->Draw(frame_time);
			for ( Bullets*  bullets : bullets_list)
			{
				bullets->Draw();
			}
			DrawPlayerScore(score);
		}
		else
		{

			putimage(0, 0, &img_menu);
			start_game_btm.Draw();
			quit_game_btm.Draw();
		}
		FlushBatchDraw();


		DWORD end_time = GetTickCount();
		DWORD delta_time = start_time - end_time;
		if (delta_time < frame_time)
		{
			Sleep(frame_time - delta_time);
		}
	}
	for (Bullets* bullet : bullets_list) {
		delete bullet;
	}
	bullets_list.clear();
	delete	atlas_enemy_left,
			atlas_enemy_right,
			atlas_player_left,
			atlas_player_right;
	EndBatchDraw();
	return 0;
}