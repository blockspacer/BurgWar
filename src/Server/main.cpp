// Copyright (C) 2018 J�r�me Leclercq
// This file is part of the "Burgwar Server" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Nazara/Network/Network.hpp>
#include <Server/BurgApp.hpp>
#include <iostream>

int main(int argc, char* argv[])
{
	Nz::Initializer<Nz::Network> network;
	bw::BurgApp app(argc, argv);
	return app.Run();
}
