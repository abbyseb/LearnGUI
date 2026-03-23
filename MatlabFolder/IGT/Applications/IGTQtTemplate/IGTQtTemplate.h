/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SimpleView.h
  Language:  C++

  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/
#ifndef IGTQtTemplate_H
#define IGTQtTemplate_H

#include <QMainWindow>

// Forward Qt class declarations
class Ui_IGTQtTemplate;

class IGTQtTemplate : public QMainWindow
{
  Q_OBJECT

public:

  // Constructor/Destructor
  IGTQtTemplate();
  ~IGTQtTemplate();

public slots:

  virtual void slotOpenFile();
  virtual void slotExit();

  void on_pushButton_released();

protected:

protected slots:

private:

  // Designer form
  Ui_IGTQtTemplate *ui;
};

#endif // IGTQtTemplate_H
