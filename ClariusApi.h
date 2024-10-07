#include <ImFusion/Core/Mat.h>

#include <functional>
#include <memory>

namespace ImFusion
{
	class IMURawMetadata;
	template <typename T>
	class TypedImage;

	class ClariusApi
	{
	public:
		virtual bool init() = 0;
		virtual bool connect(const char* ipAddress, unsigned int port) = 0;
		virtual void disconnect() = 0;
		virtual void destroy() = 0;
		virtual bool start() { return true; }
		virtual bool loadCertificate(const std::string& path) = 0;

		/// Set gain in percentage (range 0-100)
		virtual bool setGain(double gain) = 0;

		/// Set depth in millimeters
		virtual bool setDepth(double depth) = 0;

		/// Set resolution (width, height)
		virtual bool setResolution(vec2i resolution) = 0;

		std::function<void(std::unique_ptr<TypedImage<unsigned char>>&& frame, unsigned long long timestamp, std::unique_ptr<IMURawMetadata>&& imu)>
			imageCallback = {};

		std::function<void(double depth, double width)> measuresCallback = {};
		std::function<void(bool frozen)> freezeCallback = {};
		std::function<void(int btn, int clicks)> buttonCallback = {};
	};

	class ClariusCastApi : public ClariusApi
	{
	public:
		static ClariusCastApi* get();

		bool init() override;
		bool connect(const char* ipAddress, unsigned int port) override;
		void disconnect() override;
		void destroy() override;
		bool loadCertificate(const std::string& path) override;

		bool setGain(double gain) override;
		bool setDepth(double depth) override;
		bool setResolution(vec2i resolution) override;

	private:
		ClariusCastApi();
		static ClariusCastApi* m_singletonCastApiInstance;
	};
}