#include "Core/VkEngine.hpp"

int main( int argc, char** argv ){
	VkEngine e;

	e.init();
	e.run();
	e.deinit();
}
