/* 
  SZARP: SCADA software 
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
/* 
 * draw3 
 * SZARP

 *
 * $Id: wxgraphs.h 1 2009-06-24 15:09:25Z isl $
 */

#include <wx/dcbuffer.h>

#include "gcdcgraphs.h"
#include "draw.h"

GCDCGraphs::GCDCGraphs(wxWindow* parent, ConfigManager *cfg) : wxWindow(parent, wxID_ANY), m_draw_param_name(false), m_recalulate_margins(true) {
	m_screen_margins.leftmargin = 10;
	m_screen_margins.rightmargin = 10;
	m_screen_margins.topmargin = 10;
	m_screen_margins.bottommargin = 12;
	m_screen_margins.infotopmargin = 7;
	m_refresh = true;
}

double GCDCGraphs::GetX(int i) {
	int w, h;
	GetClientSize(&w, &h);

	size_t size = m_draws_wdg->GetSelectedDraw()->GetValuesTable().size();

	double res = ((double) w - m_screen_margins.leftmargin - m_screen_margins.rightmargin) / size;

	double ret = i * res + m_screen_margins.leftmargin + res / 2;

	return ret;
}

double GCDCGraphs::GetY(double value, DrawInfo *di) {
	int w, h;
	GetClientSize(&w, &h);

	h -= m_screen_margins.bottommargin + m_screen_margins.topmargin;

	double max = di->GetMax();
	double min = di->GetMin();

	double dmax = max - min;
	double dmin = 0;

	double smin = 0;
	double smax = 0;
	double k = 0;

	int sc = di->GetScale();
	if (sc) {
		smin = di->GetScaleMin() - min;
		smax = di->GetScaleMax() - min;
		assert(smax > smin);
		k = sc / 100. * (dmax - dmin) / (smax - smin);
		dmax += (k - 1) * (smax - smin);
	}

	double dif = dmax - dmin;
	if (dif <= 0) {
		wxLogInfo(_T("%s %f %f"), di->GetName().c_str(), min, max);
		assert(false);
	}

	double dvalue = value - min;

	double scaled = 
		wxMax(dvalue - smax, 0) +
		wxMax(wxMin(dvalue - smin, smax - smin), 0) * k +
		wxMax(wxMin(dvalue - dmin, smin), 0);

	double ret = h - scaled * h / dif + m_screen_margins.topmargin;

	if (ret < m_screen_margins.topmargin)
		ret = m_screen_margins.topmargin;

	if (ret > h + m_screen_margins.topmargin) 
		ret = h + m_screen_margins.topmargin;
		
	return ret;
}


const wxColour GCDCGraphs::back1_col = wxColour(128, 128, 128);
const wxColour GCDCGraphs::back2_col = wxColour(0, 0, 0);

void GCDCGraphs::DrawBackground(wxGraphicsContext &dc) {
	int w, h;
	GetClientSize(&w, &h);

	PeriodType pt = m_draws_wdg->GetSelectedDraw()->GetPeriod();
	size_t pc = m_draws_wdg->GetSelectedDraw()->GetValuesTable().size();

	dc.SetBrush(wxBrush(back2_col, wxSOLID));
	dc.DrawRectangle(0, 0, w, h);

	size_t i = 0;
	int c = 1;
	double x = m_screen_margins.leftmargin + 1;

	while (i < pc) {
		double x1 = GetX(i);

		i += Draw::PeriodMult[pt];
	
		double x2 = GetX(i);

		if (c > 0)
			dc.SetBrush(wxBrush(back1_col, wxSOLID));
		else
			dc.SetBrush(wxBrush(back2_col, wxSOLID));
	
		c *= -1; 

		double xn; 
		if (i < pc)
			xn = x + x2 - x1;
		else
			xn = w - m_screen_margins.rightmargin;

		dc.DrawRectangle(x, m_screen_margins.topmargin, x2 - x1, h - m_screen_margins.topmargin + 1);

		x = x + x2 - x1;
	}

}

wxDragResult GCDCGraphs::SetSetInfo(wxCoord x, wxCoord y, wxString window, wxString prefix, time_t time, PeriodType pt, wxDragResult def) {
	return m_draws_wdg->SetSetInfo(window, prefix, time, pt, def);
}

void GCDCGraphs::SetDrawsChanged(DrawPtrArray draws) {
	size_t pc = m_draws.GetCount();
	m_draws = draws;

	for (size_t i = pc; i < draws.GetCount(); i++)
		m_draws[i]->AttachObserver(this);

	m_recalulate_margins = true;
	Refresh();
}


void GCDCGraphs::StartDrawingParamName() {
	if (!m_draw_param_name) {
		m_draw_param_name = true;
		Refresh();
	}

}

void GCDCGraphs::StopDrawingParamName() {
	if (m_draw_param_name) {
		m_draw_param_name = false;
		Refresh();
	}
}

void GCDCGraphs::Selected(int i) {
	Refresh();
}

void GCDCGraphs::Deselected(int i) {
	Refresh();
}

void GCDCGraphs::FullRefresh() {
	Refresh();
}
	
void GCDCGraphs::SetFocus() {
	wxWindow::SetFocus();
}

void GCDCGraphs::OnSetFocus(wxFocusEvent& event) {
	SetFocus();
}

void GCDCGraphs::DrawYAxis(wxGraphicsContext& dc) {
	int w, h;
	GetClientSize(&w, &h);

	dc.StrokeLine(m_screen_margins.leftmargin, 0, m_screen_margins.leftmargin, h);
	dc.StrokeLine(m_screen_margins.leftmargin - 5, 7, m_screen_margins.leftmargin, 0);
	dc.StrokeLine(m_screen_margins.leftmargin, 0, m_screen_margins.leftmargin + 5, 7);
}

void GCDCGraphs::DrawXAxis(wxGraphicsContext &dc) {
	int arrow_width = 8;
	int arrow_height = 4;

	int w, h;
	GetClientSize(&w, &h);

	//dc.StrokeLine(0, h - m_screen_margins.bottommargin, w - m_screen_margins.rightmargin, h - m_screen_margins.bottommargin);
	dc.StrokeLine(0, h - m_screen_margins.bottommargin, w, h - m_screen_margins.bottommargin);
	dc.StrokeLine(w - arrow_width, h - m_screen_margins.bottommargin - arrow_height,
		w, h - m_screen_margins.bottommargin);
	dc.StrokeLine(w - arrow_width, h - m_screen_margins.bottommargin + arrow_height,
		w, h - m_screen_margins.bottommargin);
    
}

void GCDCGraphs::DrawYAxisVals(wxGraphicsContext& dc) {
	Draw* draw = m_draws_wdg->GetSelectedDraw();
	if (draw == NULL) 
		return;
	DrawInfo *di = draw->GetDrawInfo();

	double min = di->GetMin();
	double max = di->GetMax();
	double dif = max - min;

	if (dif <= 0) {
		wxLogInfo(_T("%s %f %f"), di->GetName().c_str(), min, max);
		assert(false);
	}

	//procedure for calculating distance between marks stolen from SzarpDraw2
	double x = dif;
	double step;
	int i = 0;

	if (x < 1)
		for (;x < 1; x *=10, --i);
	else
		for (;(int)x / 10; x /=10, ++i);

	if (x <= 1.5)
		step = .1;
	else if (x <= 3.)
		step = .2;
	else if (x <= 7.5)
		step = .5;
	else
		step = 1.;

	double acc = 1;

	int prec = di->GetPrec();
				
	for (int p = prec; p > 0; --p)
		acc /= 10;

	double factor  = (i > 0) ? 10 : .1;

	for (;i; i -= i / abs(i))
		step *= factor;

	if (step < acc)
		step = acc;

    	dc.SetPen(wxPen(*wxWHITE, 1, wxSOLID));

	int w, h;
	GetClientSize(&w, &h);

	h -= m_screen_margins.bottommargin + m_screen_margins.topmargin;

	for (double val = max; (min - val) < acc; val -= step) {

		int y = GetY(val, di);

		dc.StrokeLine(m_screen_margins.leftmargin - 8, y, m_screen_margins.leftmargin, y);

		wxString sval = di->GetValueStr(val, _T("- -"));
		double textw, texth, textd, textel;
		dc.GetTextExtent(sval, &textw, &texth, &textd, &textel);
		dc.DrawText(sval, m_screen_margins.leftmargin - textw - 1, y + 2);
	}

}

void GCDCGraphs::DrawGraphs(wxGraphicsContext &dc) {
	dc.SetPen(wxPen(wxColour(255, 255, 255), 1, wxSOLID));

	int sel = m_draws_wdg->GetSelectedDrawIndex();
	if (sel < 0)
		return;

	for (size_t i = 0; i <= m_draws.GetCount(); i++) {
		 
		size_t j = i;
		if ((int) j == sel) 
			continue;
		if (j == m_draws.GetCount())	
			j = sel;

		Draw* d = m_draws[j];

		DrawGraph(dc, d);

		if ((int) j == sel)
			DrawCursor(dc, d);
	}

}

void GCDCGraphs::DrawGraph(wxGraphicsContext &dc, Draw* d) {
	if (d->GetEnable() == false)
		return;

	DrawInfo *di = d->GetDrawInfo();

	wxGraphicsPath path = dc.CreatePath();

	const wxColour &wc = di->GetDrawColor();
	dc.SetPen(wxPen(wc, 2, wxSOLID));

	size_t pc = d->GetValuesTable().size();

	bool prev_data = false;

	for (size_t i = 0; i < pc; i++) {
		if (!d->GetValuesTable().at(i).IsData()) {
			prev_data = false;
			continue;
		}

		double x = GetX(i);
		double y = GetY(d->GetValuesTable().at(i).val, di);

		if (prev_data)
			path.AddLineToPoint(x, y);
		else
			path.MoveToPoint(x, y);
		
		prev_data = true;
	}

	dc.StrokePath(path);
}

void GCDCGraphs::DrawCursor(wxGraphicsContext &dc, Draw* d) {
	int i = d->GetCurrentIndex();
	if (i < 0)
		return;

	if (d->GetValuesTable().at(i).IsData() == false)
		return;

	dc.SetBrush(wxBrush(wxColour(0,0,0), wxTRANSPARENT));
	dc.SetPen(wxPen(*wxWHITE, 1, wxSOLID));

	double x = GetX(i);
	double y = GetY(d->GetValuesTable().at(i).val, d->GetDrawInfo());

	dc.DrawRectangle(x - 4, y - 4, 9, 9);
}

void GCDCGraphs::OnPaint(wxPaintEvent& e) {
	wxBufferedPaintDC pdc(this);
	wxGraphicsContext* dc = wxGraphicsContext::Create(pdc);

	dc->SetFont(GetFont(), *wxWHITE);
	
	RecalculateMargins(*dc);
	DrawBackground(*dc);

	dc->SetBrush(*wxWHITE_BRUSH);
	dc->SetPen(wxPen(wxColour(255, 255, 255, 255), 1, wxSOLID));
	DrawXAxis(*dc);
	//DrawXAxisVals(*dc);
	DrawYAxis(*dc);
	DrawYAxisVals(*dc);
	DrawGraphs(*dc);

	delete dc;
}

void GCDCGraphs::Refresh() {
	m_refresh = true;
}

void GCDCGraphs::RecalculateMargins(wxGraphicsContext &dc) {
	if (!m_recalulate_margins)
		return;

	double leftmargin = 36;
	double bottommargin = 12;
	double topmargin = 24;

	for (size_t i = 0; i < m_draws.GetCount(); i++) {
		double tw, th, td, tel;
		DrawInfo *di = m_draws[i]->GetDrawInfo();
		wxString sval = di->GetValueStr(di->GetMax(), _T(""));

		dc.GetTextExtent(sval, &tw, &th, &td, &tel);
		if (leftmargin < tw + 1)
			leftmargin = tw + 1;
		if (bottommargin < th + 1)
			bottommargin = th + 1;

		if (topmargin < th + m_screen_margins.infotopmargin)
			topmargin = th + m_screen_margins.infotopmargin;

	}

	m_screen_margins.leftmargin = leftmargin;
	m_screen_margins.bottommargin = bottommargin;
	m_screen_margins.topmargin = topmargin;

	m_recalulate_margins = false;
}


void GCDCGraphs::OnMouseLeftDown(wxMouseEvent& event) {

}

void GCDCGraphs::OnMouseLeftDClick(wxMouseEvent& event) {

}

void GCDCGraphs::OnMouseLeftUp(wxMouseEvent& event) {

}

void GCDCGraphs::OnMouseRightDown(wxMouseEvent& event) {

}

void GCDCGraphs::OnSize(wxSizeEvent& event) {
	Refresh();
}

void GCDCGraphs::OnEraseBackground(wxEraseEvent& event) {
	//
}

void GCDCGraphs::OnIdle(wxIdleEvent &e) {
	if (m_refresh) {
		wxWindow::Refresh();
		m_refresh = false;
	}
}

GCDCGraphs::~GCDCGraphs() {

}

BEGIN_EVENT_TABLE(GCDCGraphs, wxWindow) 
	EVT_PAINT(GCDCGraphs::OnPaint)
	EVT_IDLE(GCDCGraphs::OnIdle)
	EVT_LEFT_DOWN(GCDCGraphs::OnMouseLeftDown)
	EVT_LEFT_DCLICK(GCDCGraphs::OnMouseLeftDClick)
	EVT_LEAVE_WINDOW(GCDCGraphs::OnMouseLeftUp)
	EVT_LEFT_UP(GCDCGraphs::OnMouseLeftUp)
	EVT_SIZE(GCDCGraphs::OnSize)
	EVT_SET_FOCUS(GCDCGraphs::OnSetFocus)
	EVT_ERASE_BACKGROUND(GCDCGraphs::OnEraseBackground)
END_EVENT_TABLE()
