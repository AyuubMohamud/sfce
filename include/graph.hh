#pragma once

#include <vector>
#include <sfce.hh>
class AdjacencyMatrix {
public:
    explicit AdjacencyMatrix(u32 m_size) {
        matrix = new bool[m_size*m_size];
        adjMatrixSz = m_size*m_size;
    }
    ~AdjacencyMatrix() {
        delete matrix;
    }
    bool isConnected(u32 u, u32 v) {
        if (u*v > adjMatrixSz)
        {
            return false;
        }
#pragma clang diagnostic push
#pragma ide diagnostic ignored "NullDereference"
        return matrix[u*v]; // Null dereference not possible.
#pragma clang diagnostic pop
    }
    [[nodiscard]] u64 size() const {
        return adjMatrixSz;
    }
private:
    u64 adjMatrixSz = 0;
    bool* matrix = nullptr;
};
