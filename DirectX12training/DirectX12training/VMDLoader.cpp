#include "VMDLoader.h"
#include <algorithm>


VMDLoader::VMDLoader()
{
	FILE* fp;
	std::string path = "motion/ÉÇÅ[ÉVÉáÉì/swing2.vmd";
	fopen_s(&fp, path.c_str(), "rb");
	fseek(fp, 50, SEEK_SET);
	unsigned long cnt;
	fread(&cnt, sizeof(cnt), 1, fp);
	_vmd.motion.resize(cnt);
	for (auto&m : _vmd.motion)
	{
		fread(&m.boneName, sizeof(m.boneName[0]), _countof(m.boneName), fp);
		fread(&m.frameNo, sizeof(m.frameNo), 1, fp);
		fread(&m.location, sizeof(m.location), 1, fp);
		fread(&m.rotation, sizeof(m.rotation), 1, fp);
		fread(&m.interpolation, sizeof(m.interpolation[0]), _countof(m.interpolation), fp);
	}
	/*fread(&cnt, sizeof(cnt), 1, fp);
	_vmd.skin.resize(cnt);
	for (auto&s : _vmd.skin)
	{
		fread(&s.skinName, sizeof(s.skinName[0]), _countof(s.skinName), fp);
		fread(&s.frameNo, sizeof(s.frameNo), 1, fp);
		fread(&s.weight, sizeof(s.weight), 1, fp);
	}
	fread(&cnt, sizeof(cnt), 1, fp);
	_vmd.camera.resize(cnt);
	for (auto&c : _vmd.camera)
	{
		fread(&c.frameNo, sizeof(c.frameNo), 1, fp);
		fread(&c.length, sizeof(c.length), 1, fp);
		fread(&c.location, sizeof(c.location), 1, fp);
		fread(&c.rotation, sizeof(c.rotation), 1, fp);
		fread(&c.interpolation, sizeof(c.interpolation[0]), _countof(c.interpolation), fp);
		fread(&c.viewingAngle, sizeof(c.viewingAngle), 1, fp);
		fread(&c.perspective, sizeof(c.perspective), 1, fp);
	}
	fread(&cnt, sizeof(cnt), 1, fp);
	_vmd.light.resize(cnt);
	for (auto& l : _vmd.light)
	{
		fread(&l.frameNo, sizeof(l.frameNo), 1, fp);
		fread(&l.color, sizeof(l.color), 1, fp);
		fread(&l.location, sizeof(l.location), 1, fp);
	}
	fread(&cnt, sizeof(cnt), 1, fp);
	_vmd.shadow.resize(cnt);
	for (auto& sh : _vmd.shadow)
	{
		fread(&sh.frameNo, sizeof(sh.frameNo), 1, fp);
		fread(&sh.mode, sizeof(sh.mode), 1, fp);
		fread(&sh.distance, sizeof(sh.distance), 1, fp);
	}*/
	fclose(fp);

	for (auto& f : _vmd.motion)
	{
		_animData[f.boneName].emplace_back(KeyFrame(f.frameNo, f.rotation, f.location,
			DirectX::XMFLOAT2((float)f.interpolation[3 + 15] / 127.0f, (float)f.interpolation[7 + 15] / 127.0f),
			DirectX::XMFLOAT2((float)f.interpolation[11 + 15] / 127.0f, (float)f.interpolation[15 + 15] / 127.0f)));
		_duration = std::max(_duration, f.frameNo );
	}

	for (auto& a : _animData)
	{
		std::sort(a.second.begin(), a.second.end(),
			[](KeyFrame& a, KeyFrame& b) {return a.frameNo < b.frameNo; });
	}
}


VMDLoader::~VMDLoader()
{
}

const std::map<std::string, std::vector<KeyFrame>>& VMDLoader::GetAnim() const
{
	return _animData;
}

const int VMDLoader::GetDuration() const
{
	return _duration;
}
