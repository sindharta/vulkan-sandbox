#pragma once

#include <stdint.h>

class QueueFamilyIndices {
public :
    QueueFamilyIndices() : m_graphicsIndex(0), m_presentIndex(0), m_graphicsSet(false), m_presentSet(false) { }

    bool IsComplete() {
        return m_graphicsSet && m_presentSet;
    }

    inline bool IsGraphicsIndexSet() const;
    inline bool IsPresentIndexSet() const;

    inline void SetGraphicsIndex(uint32_t id);
    inline void SetPresentIndex(uint32_t id);
    inline uint32_t GetGraphicsIndex() const;
    inline uint32_t GetPresentIndex() const;


private:
    uint32_t m_graphicsIndex;
    uint32_t m_presentIndex;   
    bool m_graphicsSet;
    bool m_presentSet;
};

//---------------------------------------------------------------------------------------------------------------------

bool QueueFamilyIndices::IsGraphicsIndexSet()       const { return m_graphicsSet; };
bool QueueFamilyIndices::IsPresentIndexSet()   const { return m_presentSet;}
void QueueFamilyIndices::SetGraphicsIndex(uint32_t id) { m_graphicsIndex = id; m_graphicsSet = true;}
void QueueFamilyIndices::SetPresentIndex(uint32_t id) { m_presentIndex= id; m_presentSet = true;}
uint32_t QueueFamilyIndices::GetGraphicsIndex() const  { return m_graphicsIndex; }
uint32_t QueueFamilyIndices::GetPresentIndex() const   { return m_presentIndex; }


