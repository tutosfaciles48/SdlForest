#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include "util.h"
#include "Cell.h"

App app{};

bool init();

void close();

Cell* getCellAt(int x, int y);

void update();

std::vector<int> getInternalStatus(const std::vector<CellStatus>& status);

void updateYoung(Cell *cell, int nb_start_burning, int nb_burning, int nb_end_burning);
void updateMature(Cell *cell, int nb_start_burning, int nb_burning, int nb_end_burning, int nb_mature);
void updateStartBurning(Cell *cell);
void updateBurning(Cell *cell);
void updateEndBurning(Cell *cell);
void updateAshes(Cell *cell);

int randomNb(int min, int max);

std::vector<Cell*> cells;

template <typename T>
void delete_pointed_to(T* const ptr) {
	delete ptr;
}

int main(int argc, char *args[]) {
	bool quit = false;

	SDL_Event e;

	cells.reserve(nb_cells);

	//Insert each cell
	//For each line
	for(int row = 0; row < nb_rows; row++) {
		//For each col
		for(int col = 0; col < nb_cols; col++) {
			cells.push_back(new Cell(col * BLOCK_SIZE, row * BLOCK_SIZE));
		}
	}

	if(!init()) {
		std::cout << "failed to initialize" << std::endl;
	} else {
		while(!quit) {
			while(SDL_PollEvent(&e) != 0) {
				if(e.type == SDL_QUIT) {
					quit = true;
				} else if(e.type == SDL_KEYDOWN) {
					if(e.key.keysym.sym == SDLK_ESCAPE) {
						quit = true;
					}
				}
			}

			auto start = std::chrono::system_clock::now();

			//Clear screen
			SDL_SetRenderDrawColor(app.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
			SDL_RenderClear(app.renderer);

			//Draw each cell
			for(auto cell : cells) {
				cell->draw(app.renderer);
			}

			SDL_RenderPresent(app.renderer);

			update();

			auto end = std::chrono::system_clock::now();

			std::chrono::duration<double, std::milli> elapsed = end - start;

			std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(FPS - elapsed.count())));
		}
	}

	//For each cell
	for(auto c : cells) {
		delete_pointed_to(c);
	}

	close();

	return 0;
}

bool init() {
	bool success = true;

	int renderFlag, windowFlag;

	renderFlag = SDL_RENDERER_ACCELERATED;
	windowFlag = 0;

	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cerr << "Err: unable to initialize SDL : " << SDL_GetError() << std::endl;
		success = false;
	}

	app.window = SDL_CreateWindow("SDL Forest", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, windowFlag);

	if(!app.window) {
		std::cerr << "Err: unable to open window : " << SDL_GetError() << std::endl;
		success = false;
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	app.renderer = SDL_CreateRenderer(app.window, -1, renderFlag);

	if(!app.renderer) {
		std::cerr << "Renderer couldn't be created !" << SDL_GetError() << std::endl;
		success = false;
	} else {
		SDL_SetRenderDrawColor(app.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	}

	return success;
}

Cell* getCellAt(int x, int y) {
	return cells[(x * nb_cols) + y];
}

void update() {
	for(int r = 0; r < nb_rows; r++) {
		for(int c = 0; c < nb_cols; c++) {
			Cell *cell = getCellAt(r, c);

			int nb_start_burning;
			int nb_burning;
			int nb_end_burning;
			int nb_mature;

			std::vector<CellStatus> cellStatusToLook;
			cellStatusToLook.reserve(8);

			if(r > 0) {
				cellStatusToLook.push_back(getCellAt(r - 1, c)->getStatus()); //N

				if((c + 1) < nb_cols) {
					cellStatusToLook.push_back(getCellAt(r - 1, c + 1)->getStatus());//NE
				}

				if(c > 0) {
					cellStatusToLook.push_back(getCellAt(r-1, c-1)->getStatus()); //NO
				}
			}

			if(c > 0) {
				cellStatusToLook.push_back(getCellAt(r, c-1)->getStatus()); //O
			}

			if((c + 1) < nb_cols) {
				cellStatusToLook.push_back(getCellAt(r, c + 1)->getStatus()); //E
			}

			if((r + 1) < nb_rows) {
				cellStatusToLook.push_back(getCellAt(r + 1, c)->getStatus()); //S

				if((c + 1) < nb_cols) {
					cellStatusToLook.push_back(getCellAt(r + 1, c + 1)->getStatus()); //SE
				}

				if(c > 0) {
					cellStatusToLook.push_back(getCellAt(r + 1, c - 1)->getStatus()); //SO
				}
			}

			std::vector<int> statusResult = getInternalStatus(cellStatusToLook); // nb_start_burning nb_burning nb_end_burning nb_mature
			nb_start_burning = statusResult[0];
			nb_burning = statusResult[1];
			nb_end_burning = statusResult[2];
			nb_mature = statusResult[3];

			switch (cell->getStatus()) {
				case YOUNG:
					updateYoung(cell, nb_start_burning, nb_burning, nb_end_burning);
					break;
				case MATURE:
					updateMature(cell, nb_start_burning, nb_burning, nb_end_burning, nb_mature);
					break;
				case START_BURNING:
					updateStartBurning(cell);
					break;
				case BURNING:
					updateBurning(cell);
					break;
				case END_BURNING:
					updateEndBurning(cell);
					break;
				case ASHES:
					updateAshes(cell);
					break;
			}
		}
	}
}

std::vector<int> getInternalStatus(const std::vector<CellStatus>& status) {
	std::vector<int> tmp;
	tmp.reserve(4);

	int nb_start_burning = 0;
	int nb_burning = 0;
	int nb_end_burning = 0;
	int nb_mature = 0;

	for(auto s : status) {
		if(s == START_BURNING) {
			nb_start_burning++;
		}

		if(s == BURNING) {
			nb_burning++;
		}

		if(s == END_BURNING) {
			nb_end_burning++;
		}

		if(s == MATURE) {
			nb_mature++;
		}
	}

	tmp.push_back(nb_start_burning);
	tmp.push_back(nb_burning);
	tmp.push_back(nb_end_burning);
	tmp.push_back(nb_mature);

	return tmp;
}

void updateYoung(Cell *cell, int nb_start_burning, int nb_burning, int nb_end_burning) {
	if(nb_start_burning >= 1) {
		if(randomNb(1, 10) == 1) {
			cell->setStatus(START_BURNING);
		}
	} else {
		if(nb_burning >= 1) {
			if(randomNb(1, 10) <= 2) {
				cell->setStatus(START_BURNING);
			}
		} else {
			if(nb_end_burning >= 1) {
				if(randomNb(1, 10) == 1) {
					cell->setStatus(START_BURNING);
				}
			} else {
				// -> Forêt ancienne
				if(randomNb(1, 1000) <= 5) {
					cell->setStatus(MATURE);
				}
			}
		}
	}
}

void updateMature(Cell *cell, int nb_start_burning, int nb_burning, int nb_end_burning, int nb_mature) {
	if(nb_start_burning >= 1) {
		if(randomNb(1, 10) == 1) {
			cell->setStatus(START_BURNING);
		}
	} else {
		if(nb_burning >= 1) {
			if(randomNb(1, 10) <= 2) {
				cell->setStatus(START_BURNING);
			}
		} else {
			if(nb_end_burning >= 1) {
				if(randomNb(1, 10) == 1) {
					cell->setStatus(START_BURNING);
				}
			} else {
				if (nb_mature >= 5) {
					if (randomNb(1, 100000) <= 5) {
						cell->setStatus(START_BURNING);
					}
				}
			}
		}
	}
}

void updateStartBurning(Cell *cell) {
	if(randomNb(1, 10) == 1) {
		cell->setStatus(BURNING);
	}
}

void updateBurning(Cell *cell) {
	if(randomNb(1, 10) == 1) {
		cell->setStatus(END_BURNING);
	}
}

void updateEndBurning(Cell *cell) {
	if(randomNb(1, 10) == 1) {
		cell->setStatus(ASHES);
	}
}

void updateAshes(Cell *cell) {
	if(randomNb(0, 1000) <= 1) {
		cell->setStatus(YOUNG);
	}
}


int randomNb(int min, int max) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> distrib(min, max);

	return distrib(gen);
}

void close() {
	SDL_DestroyRenderer(app.renderer);
	SDL_DestroyWindow(app.window);
	app.window = nullptr;
	app.renderer = nullptr;

	IMG_Quit();
	SDL_Quit();
}