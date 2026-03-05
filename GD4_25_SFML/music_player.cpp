#include "music_player.hpp"

MusicPlayer::MusicPlayer() : m_volume(10.f)
{
	m_filenames[MusicThemes::kMenuTheme] = "Media/Music/menu_beat.mp3";
	m_filenames[MusicThemes::kGameTheme] = "Media/Music/game_beat.wav";
}


void MusicPlayer::Play(MusicThemes theme)
{
	std::string filename = m_filenames[theme];

	if (!m_music.openFromFile(filename))
	{
		throw std::runtime_error("Music " + filename + " could not be loaded.");
	}

	m_music.setVolume(m_volume);
	m_music.setLooping(true);
	m_music.setRelativeToListener(true);
	m_music.setPosition({ 0, 0, 0 });
	m_music.play();
}

void MusicPlayer::Stop()
{
	m_music.stop();
}

void MusicPlayer::SetVolume(float volume)
{
	m_volume = volume;
}

void MusicPlayer::SetPaused(bool paused)
{
	if (paused)
	{
		m_music.pause();
	} 
	else
	{
		m_music.play();
	}
}