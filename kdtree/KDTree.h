#pragma once
#include <array>
#include <memory>
#include <limits>
#include <cmath>
#include <iostream>

template <typename T, size_t N>
class Point {
public:
    explicit Point(const std::array<T, N>& p) : point(p) {}
    Point(std::initializer_list<T> init) {
        std::copy(init.begin(), init.end(), point.begin());
    }

    T operator[](size_t i) const { return point[i]; }

    bool operator==(const Point& p) const {
        return point == p.point;
    }

    bool operator!=(const Point& p) const {
        return !(*this == p);
    }

    friend std::ostream& operator<<(std::ostream& os, const Point& p) {
        os << "(";
        for (size_t i = 0; i < N; ++i) {
            os << p.point[i];
            if (i < N - 1) os << ", ";
        }
        os << ")";
        return os;
    }

    static T squaredDistance(const Point& p1, const Point& p2) {
        T sum = 0;
        for (size_t i = 0; i < N; ++i) {
            T diff = p1[i] - p2[i];
            sum += diff * diff;
        }
        return sum;
    }

private:
    std::array<T, N> point;
};

template <typename T, size_t N>
class KDTree {
    struct Node {
        Point<T, N> point;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
        explicit Node(const Point<T, N>& p) : point(p) {}
    };

    std::unique_ptr<Node> root;

    std::unique_ptr<Node> insert_(std::unique_ptr<Node> node, const Point<T, N>& p, size_t depth) {
        if (!node) return std::make_unique<Node>(p);
        size_t axis = depth % N;
        if (p[axis] < node->point[axis])
            node->left = insert_(std::move(node->left), p, depth + 1);
        else
            node->right = insert_(std::move(node->right), p, depth + 1);
        return node;
    }

    bool search_(const Node* node, const Point<T, N>& p, size_t depth) const {
        if (!node) return false;
        if (node->point == p) return true;
        return search_((p[depth % N] < node->point[depth % N]) ? node->left.get() : node->right.get(), p, depth + 1);
    }

    bool remove_(std::unique_ptr<Node>& node, const Point<T, N>& p, size_t depth) {
        if (!node) return false;
        if (node->point == p) {
            if (node->right) {
                node->point = findMin_(node->right.get(), depth % N, 0);
                remove_(node->right, node->point, depth + 1);
            } else {
                node = std::move(node->left);
            }
            return true;
        }
        return remove_((p[depth % N] < node->point[depth % N]) ? node->left : node->right, p, depth + 1);
    }

    Point<T, N> findMin_(const Node* node, size_t axis, size_t depth) const {
        if (!node) return Point<T, N>({});
        if (depth % N == axis) {
            return node->left ? findMin_(node->left.get(), axis, depth + 1) : node->point;
        }
        Point<T, N> leftMin = node->left ? findMin_(node->left.get(), axis, depth + 1) : node->point;
        Point<T, N> rightMin = node->right ? findMin_(node->right.get(), axis, depth + 1) : node->point;
        return (leftMin[axis] < rightMin[axis]) ? leftMin : rightMin;
    }

    void nearestNeighbor_(const Node* node, const Point<T, N>& target, const Node*& best, T& bestDist, size_t depth) const {
        if (!node) return;
        T d = Point<T, N>::squaredDistance(target, node->point);
        if (d < bestDist) {
            bestDist = d;
            best = node;
        }
        size_t axis = depth % N;
        const Node* nextBranch = (target[axis] < node->point[axis]) ? node->left.get() : node->right.get();
        const Node* otherBranch = (target[axis] < node->point[axis]) ? node->right.get() : node->left.get();
        nearestNeighbor_(nextBranch, target, best, bestDist, depth + 1);
        if ((target[axis] - node->point[axis]) * (target[axis] - node->point[axis]) < bestDist) {
            nearestNeighbor_(otherBranch, target, best, bestDist, depth + 1);
        }
    }

public:
    void insert(const Point<T, N>& p)
    {
        root = insert_(std::move(root), p, 0);
    }

    bool remove(const Point<T, N>& p)
    {
        return remove_(root, p, 0);
    }

    bool search(const Point<T, N>& p) const
    {
        return search_(root.get(), p, 0);
    }

    Point<T, N> searchClosestNeighbor(const Point<T, N>& p) const {
        const Node* best = nullptr;
        T bestDist = std::numeric_limits<T>::max();
        nearestNeighbor_(root.get(), p, best, bestDist, 0);
        return best ? best->point : Point<T, N>({});
    }
};
