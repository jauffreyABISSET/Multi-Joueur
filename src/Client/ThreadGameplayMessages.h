#pragma once
#include "../lib/Reseau/TThread.h"

class ThreadGameplayMessages : public TThread
{
public:
	void Function() override;
};

