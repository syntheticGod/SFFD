#pragma once

enum Draw_mode {
	normal_mode,   //���ģʽ
	mesh_mode    //����ģʽ
};

enum Deformation_mode {
	dffd_mode,   //ֱ�ӱ���ģʽ
	boundbox_mode    //��Χ�б���ģʽ
};

struct System_mode
{
	static Draw_mode draw_mode;
	static Deformation_mode deformation_mode;

};