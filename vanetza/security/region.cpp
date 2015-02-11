#include <vanetza/security/region.hpp>

using namespace std;

namespace vanetza {
namespace security {

RegionType get_type(const GeograpicRegion& reg) {
    struct geograpical_region_visitor: public boost::static_visitor<> {
        void operator()(CircularRegion reg) {
            m_type = RegionType::Circle;
        }
        void operator()(std::list<RectangularRegion> reg) {
            m_type = RegionType::Rectangle;
        }
        void operator()(PolygonalRegion reg) {
            m_type = RegionType::Polygon;
        }
        void operator()(IdentifiedRegion reg) {
            m_type = RegionType::ID;
        }
        RegionType m_type;
    };

    geograpical_region_visitor visit;
    boost::apply_visitor(visit, reg);
    return visit.m_type;
}

size_t get_size(const TwoDLocation& loc) {
    size_t size = 0;
    size += sizeof(loc.latitude);
    size += sizeof(loc.longtitude);
    return size;
}

size_t get_size(const CircularRegion& reg) {
    size_t size = 0;
    size += get_size(reg.center);
    size += sizeof(reg.radius);
    return size;
}

size_t get_size(const RectangularRegion& reg) {
    size_t size = 0;
    size += get_size(reg.nortwest);
    size += get_size(reg.southeast);
    return size;
}

size_t get_size(const std::list<CircularRegion>& list) {
    size_t size = 0;
    for (auto& circularRegion : list) {
        size += get_size(circularRegion.center);
        size += sizeof(circularRegion.radius);
    }
    return size;
}

size_t get_size(const std::list<RectangularRegion>& list) {
    size_t size = 0;
    for (auto& rectangularRegion : list) {
        size += get_size(rectangularRegion.nortwest);
        size += get_size(rectangularRegion.southeast);
    }
    return size;
}

size_t get_size(const PolygonalRegion& reg) {
    size_t size = 0;
    for (auto& twoDLocation : reg) {
        size += sizeof(twoDLocation.latitude);
        size += sizeof(twoDLocation.longtitude);
    }
    return size;
}

size_t get_size(const IdentifiedRegion& reg) {
    size_t size = 0;
    size += sizeof(reg.region_dictionary);
    size += sizeof(reg.region_identifier);
    size += get_size(reg.local_region);
    return size;
}

size_t get_size(const GeograpicRegion& reg) {
    RegionType type = get_type(reg);
    size_t size = 0;
    switch (type) {
    case RegionType::None:
        break;
    case RegionType::Circle:
        size = get_size(boost::get<CircularRegion>(reg));
        break;
    case RegionType::Rectangle:
        size = get_size(boost::get<std::list<RectangularRegion>>(reg));
        break;
    case RegionType::Polygon:
        size = get_size(boost::get<PolygonalRegion>(reg));
        break;
    case RegionType::ID:
        size = get_size(boost::get<IdentifiedRegion>(reg));
        break;
    }
    return size;
}

void serialize(OutputArchive& ar, const TwoDLocation& loc) {
    geonet::serialize(loc.latitude, ar);
    geonet::serialize(loc.longtitude, ar);
}

void serialize(OutputArchive& ar, const CircularRegion& reg) {
    serialize(ar, reg.center);
    geonet::serialize(reg.radius, ar);
}

void serialize(OutputArchive& ar, const RectangularRegion& reg) {
    serialize(ar, reg.nortwest);
    serialize(ar, reg.southeast);
}

void serialize(OutputArchive& ar, const std::list<RectangularRegion>& list) {
    size_t size;
    size = get_size(list);
    serialize_length(ar, size);
    for (auto& rectangularRegion : list) {
        serialize(ar, rectangularRegion);
    }
}

void serialize(OutputArchive& ar, const PolygonalRegion& reg) {
    size_t size;
    size = get_size(reg);
    serialize_length(ar, size);
    for (auto& twoDLocation : reg) {
        serialize(ar, twoDLocation);
    }
}

void serialize(OutputArchive& ar, const IdentifiedRegion& reg) {
    geonet::serialize(host_cast(reg.region_dictionary), ar);
    geonet::serialize(host_cast(reg.region_identifier), ar);
    serialize_length(ar, reg.local_region.get());
    serialize(ar, reg.local_region);
}

void serialize(OutputArchive& ar, const GeograpicRegion& reg) {
    struct geograpical_region_visitor: public boost::static_visitor<> {
        geograpical_region_visitor(OutputArchive& ar) :
                m_archive(ar) {
        }
        void operator()(CircularRegion reg) {
            serialize(m_archive, reg);
        }
        void operator()(std::list<RectangularRegion> reg) {
            serialize(m_archive, reg);
        }
        void operator()(PolygonalRegion reg) {
            serialize(m_archive, reg);
        }
        void operator()(IdentifiedRegion reg) {
            serialize(m_archive, reg);
        }
        OutputArchive& m_archive;
    };

    RegionType type = get_type(reg);
    ar << type;
    geograpical_region_visitor visit(ar);
    boost::apply_visitor(visit, reg);
}

size_t deserialize(InputArchive& ar, TwoDLocation& loc) {
    geonet::deserialize(loc.latitude, ar);
    geonet::deserialize(loc.longtitude, ar);
    return get_size(loc);
}

size_t deserialize(InputArchive& ar, CircularRegion& reg) {
    size_t size = 0;
    size += deserialize(ar, reg.center);
    geonet::deserialize(reg.radius, ar);
    size += sizeof(reg.radius);
    return size;
}

size_t deserialize(InputArchive& ar, std::list<RectangularRegion>& list) {
    size_t size, ret_size;
    size = deserialize_length(ar);
    ret_size = size;
    while (size > 0) {
        RectangularRegion reg;
        size -= deserialize(ar, reg.nortwest);
        size -= deserialize(ar, reg.southeast);
        list.push_back(reg);
    }
    return ret_size;
}

size_t deserialize(InputArchive& ar, PolygonalRegion& reg) {
    size_t size, ret_size;
    size = deserialize_length(ar);
    ret_size = size;
    while (size > 0) {
        TwoDLocation loc;
        size -= deserialize(ar, loc);
        reg.push_back(loc);
    }
    return ret_size;
}

size_t deserialize(InputArchive& ar, IdentifiedRegion& reg) {
    size_t size = 0;
    ar >> reg.region_dictionary;
    size += sizeof(RegionDictionary);
    geonet::deserialize(reg.region_identifier, ar);
    size += sizeof(reg.region_identifier);
    deserialize(ar, reg.local_region);
    size += get_size(reg.local_region);
    return size;
}

size_t deserialize(InputArchive& ar, GeograpicRegion& reg) {
    size_t size = 0;
    RegionType type;
    ar >> type;
    switch (type) {
    case RegionType::None:
        break;
    case RegionType::Circle: {
        CircularRegion circle;
        size = deserialize(ar, circle);
        reg = circle;
        break;
    }
    case RegionType::Rectangle: {
        std::list<RectangularRegion> list;
        size = deserialize(ar, list);
        reg = list;
        break;
    }
    case RegionType::Polygon: {
        PolygonalRegion polygon;
        size = deserialize(ar, polygon);
        reg = polygon;
        break;
    }
    case RegionType::ID: {
        IdentifiedRegion id;
        size = deserialize(ar, id);
        reg = id;
        break;
    }
    default: {
        throw deserialization_error("Unknown RegionType");
    }
    }
    return size;
}

} // namespace security
} // namespace vanetza
