/* Copyright (C) 2005-2011, Thorvald Natvig <thorvald@natvig.com>
   Copyright (C) 2010-2011, Benjamin Jemlich <pcgod@users.sourceforge.net>
   Copyright (C) 2010-2011, Stefan Hacker <dd0t@users.sourceforge.net>

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   - Neither the name of the Mumble Developers nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef MUMBLE_MUMBLE_VOICERECORDER_H_
#define MUMBLE_MUMBLE_VOICERECORDER_H_

#ifndef Q_MOC_RUN
# include <boost/scoped_ptr.hpp>
# include <boost/shared_array.hpp>
#endif

#include <sndfile.h>
#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QWaitCondition>

class ClientUser;
class RecordUser;
class Timer;

/// Utilities and enums for voice recorder format handling
namespace VoiceRecorderFormat {

	/// List of all formats currently supported by the recorder.
	enum Format {
		/// WAVE Format
		WAV = 0,
// When switching between a non vorbis capable lib and a vorbis capable one
// this can mess up the selection stored in the config
#ifndef NO_VORBIS_RECORDING
		/// Ogg Vorbis Format
		VORBIS,
#endif
		/// AU Format
		AU,
		/// FLAC Format
		FLAC,
		kEnd
	};

	/// Returns a human readable description of the format id.
	QString getFormatDescription(VoiceRecorderFormat::Format fm);

	/// Returns the default extension for the given format.
	QString getFormatDefaultExtension(VoiceRecorderFormat::Format fm);

}

/// Class for recording audio data.
///
/// Runs as a seperate thread accepting audio data through the addBuffer method
/// which is then encoded using one of the formats of VoiceRecordingFormat::Format
/// and written to disk.
///
class VoiceRecorder : public QThread {
		Q_OBJECT
	public:
		/// Possible error conditions inside the recorder
		enum Error { Unspecified, CreateDirectoryFailed, CreateFileFailed, InvalidSampleRate };

		/// Creates a new VoiceRecorder instance.
		explicit VoiceRecorder(QObject *parent);
		~VoiceRecorder();

		/// The main event loop of the thread, which writes all buffers to files.
		void run();

		/// Stops the main loop.
		void stop();

		/// Adds an audio buffer which contains |samples| audio samples to the recorder.
		/// The audio data will be aligned using the given |absoluteSampleCount|
		/// @param clientUser User for which to add the audio data. NULL in mixdown mode.
		void addBuffer(const ClientUser *clientUser, boost::shared_array<float> buffer, int samples, quint64 absoluteSampleCount);

		/// Sets the sample rate of the recorder. The sample rate can't change while the recoder is active.
		void setSampleRate(int sampleRate);
		
		/// Sets the absolute sample number considered the first sample of the recording
		void setFirstSampleAbsolute(quint64 firstSampleAbsolute);

		/// Returns the current sample rate of the encoder.
		int getSampleRate() const;

		/// Sets the path and filename for recordings.
		void setFileName(QString fn);

		/// Sets the state of multi channel recording. This can't change while the recorder is active.
		void setMixDown(bool mixDown);

		/// Returns the multi channel recording state of the recorder.
		bool getMixDown() const;

		/// Returns the elapsed time since the recording started.
		quint64 getElapsedTime() const;

		/// Returns a refence to the record user which is used to record local audio.
		RecordUser &getRecordUser() const;

		/// Sets the storage format for recordings. Can't change while the recorder is active.
		void setFormat(VoiceRecorderFormat::Format fm);

		/// Returns the current recording format.
		VoiceRecorderFormat::Format getFormat() const;
		
	signals:
		/// Emitted if an error is encountered
		void error(int err, QString strerr);

		/// Emitted when recording is started
		void recording_started();
		/// Emitted when recording is stopped
		void recording_stopped();
		
	private:
		/// Removes invalid characters in a path component.
		QString sanitizeFilenameOrPathComponent(const QString &str) const;

		/// Expands the template variables in |path| for the given |userName|.
		QString expandTemplateVariables(const QString &path, const QString& userName) const;

		/// Returns the RecordInfo hashmap index for the given user
		int indexForUser(const ClientUser *clientUser) const;
		
		/// Create a sndfile SF_INFO structure describing the currently configured recording format
		SF_INFO createSoundFileInfo() const;
		
		/// Stores information about a recording buffer.
		struct RecordBuffer {
			/// Constructs a new RecordBuffer object.
			RecordBuffer(int recordInfoIndex,
			             boost::shared_array<float> buffer,
			             int samples,
			             quint64 absoluteStartSample);

			/// Hashmap index for the user
			const int recordInfoIndex;

			/// The buffer.
			boost::shared_array<float> buffer;

			/// The number of samples in the buffer.
			int samples;

			/// Absolute sample number at the start of this buffer
			quint64 absoluteStartSample;
		};

		/// Stores the recording state for one user.
		struct RecordInfo {
			RecordInfo(quint64 lastWrittenAbsoluteSample, const QString& userName);
			~RecordInfo();

			/// Name of the user being recorded
			const QString userName;
			
			/// libsndfile's handle.
			SNDFILE *soundFile;

			/// The last absolute sample we wrote for this users
			quint64 lastWrittenAbsoluteSample;
		};

		/// Hash which maps the |uiSession| of all users for which we have to keep a recording state to the corresponding RecordInfo object.
		QHash< int, boost::shared_ptr<RecordInfo> > m_recordInfo;

		/// List containing all unprocessed RecordBuffer objects.
		QList< boost::shared_ptr<RecordBuffer> > m_recordBuffer;

		/// The user which is used to record local audio.
		boost::scoped_ptr<RecordUser> m_recordUser;

		/// High precision timer for buffer timestamps.
		boost::scoped_ptr<Timer> m_timestamp;

		/// Protects the buffer list |qlRecordBuffer|.
		QMutex m_bufferLock;

		/// Wait condition and mutex to block until there is new data.
		QMutex m_sleepLock;
		QWaitCondition m_sleepCondition;

		/// The current sample rate of the recorder.
		int m_sampleRate;

		/// True if the main loop is active.
		bool m_recording;

		/// The path to store recordings.
		QString m_fileName;

		/// True if multi channel recording is disabled.
		bool m_mixDownMode;

		/// The current recording format.
		VoiceRecorderFormat::Format m_recordingFormat;

		/// The timestamp where the recording started.
		const QDateTime m_recordingStartTime;

		/// Absolute sample number which to consider the start of the recording
		quint64 m_firstSampleAbsolute;
};

typedef boost::shared_ptr<VoiceRecorder> VoiceRecorderPtr;

#endif
