/////////////////////////////////////////////////////////////////////////////
// Name:        doc.cpp
// Purpose:     Implements document functionality
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: doc.cpp 56679 2008-11-04 17:47:07Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows license
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include "StdAfx.h"
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if !wxUSE_DOC_VIEW_ARCHITECTURE
#error You must set wxUSE_DOC_VIEW_ARCHITECTURE to 1 in setup.h!
#endif

#if wxUSE_STD_IOSTREAM
    #include "wx/ioswrap.h"
#else
    #include "wx/txtstrm.h"
#endif

#include "doc.h"
#include "view.h"

IMPLEMENT_DYNAMIC_CLASS(DrawingDocument, wxDocument)

DrawingDocument::~DrawingDocument(void)
{
  WX_CLEAR_LIST(wxList, doodleSegments);
}

wxOutputStream& DrawingDocument::SaveObject(wxOutputStream& stream)
{
  wxDocument::SaveObject(stream);

  wxTextOutputStream text_stream( stream );

  wxInt32 n = doodleSegments.GetCount();
  text_stream << n << _T('\n');

  wxList::compatibility_iterator node = doodleSegments.GetFirst();
  while (node)
  {
    DoodleSegment *segment = (DoodleSegment *)node->GetData();
    segment->SaveObject(stream);
    text_stream << _T('\n');

    node = node->GetNext();
  }

  return stream;
}



wxInputStream& DrawingDocument::LoadObject(wxInputStream& stream)
{
  wxDocument::LoadObject(stream);

  wxTextInputStream text_stream( stream );

  wxInt32 n = 0;
  text_stream >> n;

  for (int i = 0; i < n; i++)
  {
    DoodleSegment *segment = new DoodleSegment;
    segment->LoadObject(stream);
    doodleSegments.Append(segment);
  }

  return stream;
}


DoodleSegment::DoodleSegment(const DoodleSegment& seg)
              :wxObject()
{
  wxList::compatibility_iterator node = seg.lines.GetFirst();
  while (node)
  {
    DoodleLine *line = (DoodleLine *)node->GetData();
    DoodleLine *newLine = new DoodleLine;
    newLine->x1 = line->x1;
    newLine->y1 = line->y1;
    newLine->x2 = line->x2;
    newLine->y2 = line->y2;

    lines.Append(newLine);

    node = node->GetNext();
  }
}

DoodleSegment::~DoodleSegment(void)
{
  WX_CLEAR_LIST(wxList, lines);
}


wxOutputStream &DoodleSegment::SaveObject(wxOutputStream& stream)
{
  wxTextOutputStream text_stream( stream );

  wxInt32 n = lines.GetCount();
  text_stream << n << _T('\n');

  wxList::compatibility_iterator node = lines.GetFirst();
  while (node)
  {
    DoodleLine *line = (DoodleLine *)node->GetData();
    text_stream << line->x1 << _T(" ") <<
                   line->y1 << _T(" ") <<
           line->x2 << _T(" ") <<
           line->y2 << _T("\n");
    node = node->GetNext();
  }

  return stream;
}


wxInputStream &DoodleSegment::LoadObject(wxInputStream& stream)
{
  wxTextInputStream text_stream( stream );

  wxInt32 n = 0;
  text_stream >> n;

  for (int i = 0; i < n; i++)
  {
    DoodleLine *line = new DoodleLine;
    text_stream >> line->x1 >>
                   line->y1 >>
           line->x2 >>
           line->y2;
    lines.Append(line);
  }

  return stream;
}

void DoodleSegment::Draw(wxDC *dc)
{
  wxList::compatibility_iterator node = lines.GetFirst();
  while (node)
  {
    DoodleLine *line = (DoodleLine *)node->GetData();
    dc->DrawLine(line->x1, line->y1, line->x2, line->y2);
    node = node->GetNext();
  }
}

DrawingCommand::DrawingCommand(const wxString& name, int command, DrawingDocument *ddoc, DoodleSegment *seg):
  wxCommand(true, name)
{
  doc = ddoc;
  segment = seg;
  cmd = command;
}

DrawingCommand::~DrawingCommand(void)
{
  if (segment)
    delete segment;
}

bool DrawingCommand::Do(void)
{
  switch (cmd)
  {
    case DOODLE_CUT:
    {
      // Cut the last segment
      if (doc->GetDoodleSegments().GetCount() > 0)
      {
        wxList::compatibility_iterator node = doc->GetDoodleSegments().GetLast();
        if (segment)
          delete segment;

        segment = (DoodleSegment *)node->GetData();
        doc->GetDoodleSegments().Erase(node);

        doc->Modify(true);
        doc->UpdateAllViews();
      }
      break;
    }
    case DOODLE_ADD:
    {
      doc->GetDoodleSegments().Append(new DoodleSegment(*segment));
      doc->Modify(true);
      doc->UpdateAllViews();
      break;
    }
  }
  return true;
}

bool DrawingCommand::Undo(void)
{
  switch (cmd)
  {
    case DOODLE_CUT:
    {
      // Paste the segment
      if (segment)
      {
        doc->GetDoodleSegments().Append(segment);
        doc->Modify(true);
        doc->UpdateAllViews();
        segment = (DoodleSegment *) NULL;
      }
      doc->Modify(true);
      doc->UpdateAllViews();
      break;
    }
    case DOODLE_ADD:
    {
      // Cut the last segment
      if (doc->GetDoodleSegments().GetCount() > 0)
      {
        wxList::compatibility_iterator node = doc->GetDoodleSegments().GetLast();
        DoodleSegment *seg = (DoodleSegment *)node->GetData();
        delete seg;
        doc->GetDoodleSegments().Erase(node);

        doc->Modify(true);
        doc->UpdateAllViews();
      }
    }
  }
  return true;
}
