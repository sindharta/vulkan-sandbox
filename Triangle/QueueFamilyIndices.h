#pragma once

#include <stdint.h>

class QueueFamilyIndices {
public :
    QueueFamilyIndices() : m_graphicsIndex(0), m_presentationIndex(0), m_graphicsSet(false), m_presentationSet(false) { }

    bool IsComplete() {
        return m_graphicsSet && m_presentationSet;
    }

    inline bool IsGraphicsIndexSet() const;
    inline bool IsPresentationIndexSet() const;

    inline void SetGraphicsIndex(uint32_t id);
    inline void SetPresentationIndex(uint32_t id);
    inline uint32_t GetGraphicsIndex() const;
    inline uint32_t GetPresentationIndex() const;


private:
    uint32_t m_graphicsIndex;
    uint32_t m_presentationIndex;   
    bool m_graphicsSet;
    bool m_presentationSet;
};

//---------------------------------------------------------------------------------------------------------------------

bool QueueFamilyIndices::IsGraphicsIndexSet()       const { return m_graphicsSet; };
bool QueueFamilyIndices::IsPresentationIndexSet()   const { return m_presentationSet;}
void QueueFamilyIndices::SetGraphicsIndex(uint32_t id) { m_graphicsIndex = id; m_graphicsSet = true;}
void QueueFamilyIndices::SetPresentationIndex(uint32_t id) { m_presentationIndex= id; m_presentationSet = true;}
uint32_t QueueFamilyIndices::GetGraphicsIndex() const       { return m_graphicsIndex; }
uint32_t QueueFamilyIndices::GetPresentationIndex() const   { return m_presentationIndex; }


