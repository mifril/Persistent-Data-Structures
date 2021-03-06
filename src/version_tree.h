#ifndef VERSION_TREE_H
#define VERSION_TREE_H

#include <iostream>
#include <list>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <limits>
#include <algorithm>
#include <iterator>

class VersionTree {
private:
    struct Node {
        long version;
        size_t label;

        Node() : version(NONE_VERSION)
        {}
        Node(const Node& other) : version(other.version)
        {}
        Node(const long & version_)
            : version(version_)
        {}

        bool operator==(const Node& other) {
            return version == other.version;
        }
        bool operator==(const Node& other) const {
            return version == other.version;
        }
    };

public:
    VersionTree() : _labelsNumber(2), _labelToVersion(_labelsNumber, NONE_VERSION) {
        _init();
    }

    VersionTree(const VersionTree & other) : _events(other._events), _labelsNumber(other._labelsNumber),
            _labelToVersion(other._labelToVersion), _versionToLabel(other._versionToLabel)
    {}

    bool operator==(const VersionTree& other) {
        return _events == other._events && _labelsNumber == other._labelsNumber
                && _labelToVersion == other._labelToVersion && _versionToLabel == other._versionToLabel;
    }
    bool operator==(const VersionTree& other) const {
        return _events == other._events && _labelsNumber == other._labelsNumber
                && _labelToVersion == other._labelToVersion && _versionToLabel == other._versionToLabel;
    }
    bool operator!=(const VersionTree& other) {
        return !operator==(other);
    }
    bool operator!=(const VersionTree& other) const {
        return !operator==(other);
    }

    /* insert version after parentVersion */
    void insert(const long & version, const long parentVersion) {
        if (_events.empty()) {
            throw new std::out_of_range("Empty version tree");
        }
        auto it = _events.begin();
        for (; it != _events.end(); ++it) {
            if (it->version == parentVersion) {
                auto pos = _insert(version, it);
                pos = _insert(-1 * version, pos);
                break;
            }
        }
        if (it == _events.end()) {
            throw new std::out_of_range("Version tree doesn't contain parent version " + parentVersion);
        }
    }

    /* if lv <= rv returns true, else false */
    bool order(const long lv, const long rv) const {
        return _getLabel(lv) <= _getLabel(rv) && _getLabel(-1 * rv) <= _getLabel(-1 * lv);
    }

    // empty version tree's _events contains only 2 entries for starting version
    bool empty() const {
        return _events.size() == 2;
    }

    size_t size() const {
        return _events.size() / 2;
    }

    void clear() {
        _events.clear();
        _init();
    }

private:
    std::list<Node> _events;
    size_t _labelsNumber;
    std::vector<long> _labelToVersion;
    std::unordered_map<long, size_t> _versionToLabel;

    static const long NONE_VERSION;
    static const double OVERFLOW_THRESHOLD_BASE;

    std::list<Node>::iterator _insert(const long version, const std::list<Node>::iterator & prev) {
        size_t prevLabel = _getLabel(prev->version);
        auto next = prev;
        ++next;
        size_t nextLabel = _getLabel(next->version);
        if (next == _events.end()) {
            nextLabel = _labelsNumber - 1;
        }

        auto pos = _events.insert(next, Node(version));

        if (nextLabel - prevLabel < 2) {
            _relabel(prevLabel, nextLabel);
            prevLabel = _getLabel(prev->version);
            nextLabel = _getLabel(next->version);
        }
        size_t label = prevLabel + (nextLabel - prevLabel + 1) / 2;

        _labelToVersion[label] = version;
        _versionToLabel[version] = label;

        return pos;
    }

    void remove(const long version) {
        bool wasDelete = false;
        for (auto it = _events.begin(); it != _events.end(); ++it) {
            if (it->version == version) {
                auto next = it;
                ++next;
                _events.erase(it);
                it = next;
                // _events always contains 2 entries of one version with diferent inEvent flag values
                if (wasDelete) {
                    break;
                }
                wasDelete = true;
            }
        }
    }

    void _relabel(const size_t firstLabel, const size_t secondLabel) {
        size_t rangeSize = 2;
        while (rangeSize <= _labelsNumber) {
            size_t firstRangeNum = firstLabel / rangeSize;
            size_t secondRangeNum = secondLabel / rangeSize;
            if (firstRangeNum == secondRangeNum) {
                size_t rangeStart = rangeSize * firstRangeNum;
                size_t rangeEnd = rangeSize * (firstRangeNum + 1);
                double rangeDensity = _getRangeDensity(rangeStart, rangeEnd);

                double overflowThreshold = std::pow(OVERFLOW_THRESHOLD_BASE, -1 * (long long)rangeSize);
                if (rangeDensity < overflowThreshold) {
                    _relabelRange(rangeStart, rangeEnd);
                    break;
                }
            }
            rangeSize *= 2;
        }
        if (rangeSize >= _labelsNumber) {
            _relabelAll();
        }
    }

    double _getRangeDensity(const size_t rangeStart, const size_t rangeEnd) {
        size_t occupied = 0;
        for (size_t i = rangeStart; i < rangeEnd; ++i) {
            if (_labelToVersion[i] != NONE_VERSION) {
                ++occupied;
            }
        }
        return (occupied + 0.0) / (rangeEnd - rangeStart);
    }

    void _relabelRange(const size_t rangeStart, const size_t rangeEnd) {
        std::list<long> rangeVersions;
        for (size_t i = rangeStart; i < rangeEnd; ++i) {
            if (_labelToVersion[i] != NONE_VERSION) {
                rangeVersions.push_back(_labelToVersion[i]);
            }
        }

        auto rangeStartIt = _labelToVersion.begin();
        std::advance(rangeStartIt, rangeStart);
        auto rangeEndIt = _labelToVersion.begin();
        std::advance(rangeEndIt, rangeEnd);
        std::fill(rangeStartIt, rangeEndIt, NONE_VERSION);

        size_t step = rangeEnd - rangeStart;
        for (size_t i = rangeStart; i < rangeEnd; i += step) {
            if (rangeVersions.empty()) {
                break;
            }
            auto version = rangeVersions.front();
            _labelToVersion[i] = version;
            _versionToLabel[version] = i;
            rangeVersions.pop_front();
        }
        _versionToLabel[NONE_VERSION] = _labelsNumber - 1;
    }

    void _relabelAll() {
        std::list<long> rangeVersions;
        for (auto version : _labelToVersion) {
            if (version != NONE_VERSION) {
                rangeVersions.push_back(version);
            }
        }

        _labelsNumber *= 2;
        _labelToVersion.resize(_labelsNumber, NONE_VERSION);
        std::fill(_labelToVersion.begin(), _labelToVersion.end(), NONE_VERSION);

        size_t step = _labelsNumber / rangeVersions.size();
        for (size_t i = 0; i < _labelsNumber; i += step) {
            if (rangeVersions.empty()) {
                break;
            }
            auto version = rangeVersions.front();
            _labelToVersion[i] = version;
            _versionToLabel[version] = i;
            rangeVersions.pop_front();
        }
        _versionToLabel[NONE_VERSION] = _labelsNumber - 1;
    }

    size_t _getLabel(const long version) const {
        return _versionToLabel.at(version);
    }

    void _init() {
        _events.push_back(Node(0));
        _events.push_back(Node(NONE_VERSION));
        _labelToVersion[0] = 0;
        _versionToLabel[0] = 0;
        _labelToVersion[_labelsNumber - 1] = NONE_VERSION;
        _versionToLabel[NONE_VERSION] = _labelsNumber - 1;
    }
};

#endif // VERSION_TREE_H
