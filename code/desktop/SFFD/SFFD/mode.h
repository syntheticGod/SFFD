#pragma once

enum Draw_mode {
	normal_mode,   //填充模式
	mesh_mode    //网格模式
};

enum Deformation_mode {
	dffd_mode,   //直接变形模式
	boundbox_mode    //包围盒变形模式
};

struct System_mode
{
	static Draw_mode draw_mode;
	static Deformation_mode deformation_mode;

};