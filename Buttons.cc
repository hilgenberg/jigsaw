#include "Buttons.h"
#include "Camera.h"
#include "Utility/Preferences.h"

void Buttons::reshape(Camera &camera)
{
	int W = camera.screen_w(), H = camera.screen_h();
	if (W <= 0) W = 1;
	if (H <= 0) H = 1;
	float w = 0.15f * exp(3.0*Preferences::button_scale()), h = w, spc = h*0.5f;
	if (W < H) h = w*W/H; else w = h*H/W;
	const int nspc = 2, nb = 9;
	P2f p(0.0, 0.0), dp(0.0, 0.0), ds(0.0, 0.0);

	switch (Preferences::button_edge())
	{
		case LEFT:
		case RIGHT:
		{
			w = h*H/W; if (w > 2.0f) { w = 2.0f; h = w*W/H; }

			if (nb*h + nspc*spc > 2.0f)
			{
				if (nspc > 0) spc = (2.0f - nb*h)/nspc; else spc = 0.0f;
				if (spc < 0.0f) spc = 0.0f;
			}

			if (nb*h + nspc*spc > 2.0f)
			{
				assert(spc == 0.0f);
				h = 2.0f/nb;
				w = h*H/W; assert(w <= 2.0f);
			}

			p.x = Preferences::button_edge() == LEFT ? -1.0+w*0.5f : 1.0-w*0.5f;
			dp.set(0.0, -h);
			ds.set(0.0, -spc);
			switch (Preferences::button_align())
			{
				case TOP_OR_LEFT: p.y = 1.0f-h*0.5f; break;
				case CENTERED: p.y = 0.5*(nb*h + nspc*spc)-h*0.5f; break;
				case BOTTOM_OR_RIGHT: p.y = -1.0f-h*0.5f + nb*h + nspc*spc; break;
			}
			break;
		}
		case TOP:
		case BOTTOM:
		{
			h = w*W/H; if (h > 2.0f) { h = 2.0f; w = h*H/W; }

			if (nb*w + nspc*spc > 2.0f)
			{
				if (nspc > 0) spc = (2.0f - nb*w)/nspc; else spc = 0.0f;
				if (spc < 0.0f) spc = 0.0f;
			}

			if (nb*w + nspc*spc > 2.0f)
			{
				assert(spc == 0.0f);
				w = 2.0f/nb;
				h = w*W/H; assert(h <= 2.0f);
			}

			p.y = Preferences::button_edge() == TOP ? 1.0-h*0.5f : -1.0+h*0.5f;
			dp.set(w, 0.0);
			ds.set(spc, 0.0);
			switch (Preferences::button_align())
			{
				case TOP_OR_LEFT: p.x = -1.0f+w*0.5f; break;
				case CENTERED: p.x = -0.5*(nb*w + nspc*spc)+w*0.5f; break;
				case BOTTOM_OR_RIGHT: p.x = 1.0f+w*0.5f - nb*w - nspc*spc; break;
			}
			break;
		}

		default: assert(false);
	}

	button_size.set(w, h, spc);
	buttons.resize(nb); int i = 0;
	#define B(x) do{ assert(i < nb); auto &b = buttons[i++]; b.index = x; b.pos = p; p += dp; }while(0)
	B(RESET_VIEW);
	B(ARRANGE);
	B(EDGE_ARRANGE);
	p += ds;
	B(CHANGE_IMAGE);
	B(SETTINGS);
	B(PREFERENCES);
	p += ds;
	B(HIDE);
	B(SHOVEL);
	B(MAGNET);
	#undef B
	assert(i == nb);
}
