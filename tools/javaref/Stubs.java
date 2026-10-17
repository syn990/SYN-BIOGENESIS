/* SYN-BIOGENESIS: Biogenesis 0.9 rewritten in C.
 * Copyright (C) 2026 syn990
 *
 * Part of a C port of Biogenesis (https://biogenesis.sourceforge.net/).
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version. It is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License (LICENSE) for more details.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Stand-ins for the Swing side of Biogenesis 0.9, so the original
 * simulation classes (World, Organism, GeneticCode, Gene, Utils, ...) compile
 * and run headless. Only what those classes touch is stubbed; every method
 * is a no-op because nothing on screen affects the simulation. */
package biogenesis;

import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Rectangle;
import java.util.List;

class Messages {
	static String getString(String key) { return key; }
	static String getLanguage() { return "en"; }
	static void setLocale(String l) {}
	static void setLocale(int i) {}
	static Integer getMnemonic(String key) { return 0; }
}

class InfoToolbar {
	void recalculate() {}
	void changeNChildren() {}
	void changeNKills() {}
	void changeNInfected() {}
}

class MainWindow {
	InfoToolbar info = new InfoToolbar();
	InfoToolbar getInfoPanel() { return info; }
	int getX() { return 0; }
	int getY() { return 0; }
	int getWidth() { return 0; }
	int getHeight() { return 0; }
	int getExtendedState() { return 0; }
}

class VisibleWorld {
	MainWindow _mainWindow = new MainWindow();
	Organism selected;
	void setPreferredSize(Dimension d) {}
	void repaint(Rectangle r) {}
	Organism getSelectedOrganism() { return selected; }
	void setSelectedOrganism(Organism o) { selected = o; }
	void showDeadToolbar() {}
}

class StatisticsWindow {
	StatisticsWindow(MainWindow w, WorldStatistics ws, List<Organism> os) {}
}

class Corridor extends Rectangle {
	private static final long serialVersionUID = 1L;
	void draw(Graphics g) {}
	Organism getTravellingOrganism() { return null; }
}

class InCorridor extends Corridor {
	private static final long serialVersionUID = 1L;
	void frame() {}
}

class OutCorridor extends Corridor {
	private static final long serialVersionUID = 1L;
	boolean canSendOrganism() { return false; }
	boolean sendOrganism(Organism o) { return false; }
}
