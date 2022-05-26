#include "ButtonSetter.h"

ButtonSetter::ButtonSetter(QWidget* pwgt)
	: QLineEdit(pwgt), key(-1)
{
	setReadOnly(true);
}

void ButtonSetter::keyPressEvent(QKeyEvent* e)
{
	key = e->key();
	setText(QKeySequence(key).toString());
}

int ButtonSetter::getKey()
{
	return key;
}