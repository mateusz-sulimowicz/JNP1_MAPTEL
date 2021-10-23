#include "maptel.h"
#include <unordered_map>
#include <string>

namespace {

#ifdef NDEBUG
    static const bool debug = false;
#else
    static const bool debug = true;
#endif

    using std::hex;
    using std::string;
    using std::unordered_map;
    using std::unique_ptr;
    using std::clog;
    using std::to_string;

    // To jest słownik obsługujący zmiany numerów telefonów.
    using maptel = unordered_map<string, string>;
    // To jest identyfikator słownika.
    using maptel_id = unsigned long;
    // To jest repozytorium słowników.
    using maptel_repo = unordered_map<maptel_id, maptel>;

    // Przyszły identyfikator nowego słownika.
    maptel_id new_maptel_id = 0;

    // Ochrona przed "static initialization fiasco".
    maptel_repo &repository() {
        static unique_ptr<maptel_repo> database(new maptel_repo());
        return *database;
    }

    // Sprawdza, czy repozytorium zawiera słownik o numerze `id`.
    bool repo_contains(maptel_id id) {
        return repository().find(id) != repository().end();
    }

    // Sprawdza, czy słownik o numerze `id` zawiera klucz `tel`.
    bool maptel_contains(maptel_id id, const string &tel) {
        maptel &m = repository()[id];
        return m.find(tel) != m.end();
    }

    void log_message(const string &function, const string &message) {
        if (debug) {
            clog << "maptel: " << function << ": " << message << "\n";
        }
    }

    // -- Funkcje do logowania parametrów wywołania --

    void log_params(const string &function) {
        if (debug) {
            clog << "maptel: " << function << "()" << "\n";
        }
    }

    void log_params(const string &function, maptel_id id) {
        if (debug) {
            clog << "maptel: " << function << "(" << id << ")" << "\n";
        }
    }

    void log_params(const string &function, maptel_id id, const char *tel1) {
        if (debug) {
            clog << "maptel: " << function << "(" << id << ", " << tel1 << ")"
                 << "\n";
        }
    }

    void log_params(const string &function, maptel_id id, const char *tel1,
                    const char *tel2) {
        if (debug) {
            clog << "maptel: " << function
                 << "(" << id << ", " << tel1 << ", " << tel2 << ")"
                 << "\n";
        }
    }

    void log_params(const string &function, maptel_id id, const char *tel1,
                    char *tel2, size_t len) {
        if (debug) {
            clog << "maptel: " << function
                 << "(" << id << ", " << tel1 << ", "
                 << /*reinterpret_cast<void *>(tel2)*/"ADDR" << ", "
                 << len << ")" << "\n";
        }
    }

    // Sprawdza, czy numer telefonu jest poprawny.
    bool is_valid_tel(char const *tel) {
        if (tel == NULL) {
            return false;
        }

        size_t i = 0;
        for (; i < jnp1::TEL_NUM_MAX_LEN + 1; ++i) {
            if (!std::isdigit(tel[i])) {
                break;
            }
        }

        return tel[i] == '\0';
    }

    bool
    is_valid_output(const string &tel_src, const char *tel_dst, size_t len) {
        // + 1 na terminalne 0
        return tel_dst != NULL && len >= tel_src.length() + 1;
    }

    void persist_transformation(const string &transformed_src, char *tel_dst,
                                size_t len) {
        assert(is_valid_output(transformed_src, tel_dst, len));

        size_t output_size =
                transformed_src.length() + 1 < len
                ? transformed_src.length()
                : len - 1;
        for (size_t i = 0; i < output_size; ++i) {
            tel_dst[i] = transformed_src[i];
        }
        tel_dst[output_size] = '\0';
    }

    bool isTransformationCyclic(maptel_id id, const string &tel_src) {
        assert(repo_contains(id));

        if (!maptel_contains(id, tel_src)) {
            return false;
        }

        // Algortym żółw i zając do szukania cykli w liście.
        maptel &m = repository()[id];
        string turtle = string(tel_src);
        string hare = m[turtle];
        bool shouldTurtleMove = false;

        while (maptel_contains(id, hare) && turtle != hare) {
            hare = m[hare];
            turtle = shouldTurtleMove ? m[turtle] : turtle;
            shouldTurtleMove = !shouldTurtleMove;
        }

        return hare == turtle;
    }

    string transform(maptel_id id, const string &tel_src) {
        assert(!isTransformationCyclic(id, tel_src));
        maptel &m = repository()[id];
        string result = string(tel_src);
        while (maptel_contains(id, result)) {
            result = m[result];
        }
        return result;
    }

} // anonymous namespace

unsigned long jnp1::maptel_create(void) {
    log_params(__FUNCTION__);
    repository()[new_maptel_id];
    log_message(__FUNCTION__, "new map id = " + to_string(new_maptel_id));
    return new_maptel_id++;
}

void jnp1::maptel_delete(unsigned long id) {
    assert(repo_contains(id));
    log_params(__FUNCTION__, id);

    if (!repo_contains(id)) {
        log_message(__FUNCTION__, "nothing to delete");
        return;
    }

    repository().erase(id);
    log_message(__FUNCTION__, "map " + to_string(id) + " deleted");
}

void
jnp1::maptel_insert(unsigned long id, char const *tel_src,
                    char const *tel_dst) {
    assert(repo_contains(id) && is_valid_tel(tel_src) && is_valid_tel(tel_dst));
    log_params(__FUNCTION__, id, tel_src, tel_dst);

    string src = string(tel_src);
    string dst = string(tel_dst);
    repository()[id][src] = dst;
    log_message(__FUNCTION__, "inserted");
}

void jnp1::maptel_erase(unsigned long id, char const *tel_src) {
    assert(repo_contains(id) && is_valid_tel(tel_src));
    log_params(__FUNCTION__, id, tel_src);

    string src = string(tel_src);
    if (!maptel_contains(id, src)) {
        log_message(__FUNCTION__, "nothing to erase");
        return;
    }

    repository()[id].erase(src);
    log_message(__FUNCTION__, "erased");
}

void
jnp1::maptel_transform(unsigned long id, char const *tel_src, char *tel_dst,
                       size_t len) {
    assert(repo_contains(id) && is_valid_tel(tel_src) && tel_dst != NULL);
    log_params(__FUNCTION__, id, tel_src, tel_dst, len);

    string src = string(tel_src);
    string transformed_src;
    if (isTransformationCyclic(id, src)) {
        transformed_src = src;
        log_message(__FUNCTION__, "cycle detected");
    } else {
        transformed_src = transform(id, tel_src);
    }

    persist_transformation(transformed_src, tel_dst, len);
    log_message(__FUNCTION__, src + " -> " + transformed_src);
}