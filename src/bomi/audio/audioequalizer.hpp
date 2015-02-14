#ifndef AUDIOEQUALIZER_HPP
#define AUDIOEQUALIZER_HPP

class AudioEqualizer {
    static constexpr int BandCount = 10;
public:
    enum Preset {
        Flat, Classic, Club, Dance, FullBass, FullBassTreble,
        FullTreble, Headphones, LargeHall, Live, Party, Pop, Reggae, Rock,
        Ska, Soft, SoftRock, Techno,
        MaxPreset
    };

    AudioEqualizer() { std::fill(m_dbs.begin(), m_dbs.end(), 0.0); }
    AudioEqualizer(Preset preset): AudioEqualizer(prepare(preset)) { }
    AudioEqualizer(std::initializer_list<double> list)
        : AudioEqualizer() {
        Q_ASSERT(list.size() <= m_dbs.size());
        std::copy(list.begin(), list.end(), m_dbs.begin());
    }
    ~AudioEqualizer() { }
    auto operator == (const AudioEqualizer &rhs) const -> bool {
        for (int i = 0; i < rhs.size(); ++i) {
            if (m_dbs[i] != rhs.m_dbs[i])
                return false;
        }
        return true;
    }
    auto operator != (const AudioEqualizer &rhs) const -> bool
        { return !operator == (rhs); }
    auto operator [] (int band) const -> double { return dB(band); }
    auto operator [] (int band) -> double& { return m_dbs[band]; }
    static constexpr auto bands() -> int { return BandCount; }
    static constexpr auto size() -> int { return bands(); }
    static constexpr auto count() -> int { return bands(); }
    static auto freqeuncy(int band) -> double { return s_freqs[band]; }
    auto dB(int band) const -> double { return m_dbs[band]; }
    auto setGain(int band, double gain) -> void { m_dbs[band] = gain; }
    static constexpr auto max() -> double { return 20.0; }
    static constexpr auto min() -> double { return -max(); }
    auto isZero() const -> bool
        { for (auto v : m_dbs) { if (v != 0.0) return false; } return true; }
    static auto name(Preset preset) -> QString;
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
    static auto fromJson(const QJsonObject &json) -> AudioEqualizer;
private:
    static auto prepare(Preset preset) -> std::initializer_list<double>;
    static std::array<double, BandCount> s_freqs;
    std::array<double, BandCount> m_dbs;
};

Q_DECLARE_METATYPE(AudioEqualizer)

#endif // AUDIOEQUALIZER_HPP
