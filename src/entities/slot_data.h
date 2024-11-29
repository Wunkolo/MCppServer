#ifndef SLOT_DATA_H
#define SLOT_DATA_H
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

enum class ComponentType : size_t {
    CustomData = 0,
    MaxStackSize = 1,
    MaxDamage = 2,
    Damage = 3,
    Unbreakable = 4,
    CustomName = 5,
    ItemName = 6,
    Lore = 7,
    Rarity = 8,
    Enchantments = 9,
    CanPlaceOn = 10,
    CanBreak = 11,
    AttributeModifiers = 12,
    CustomModelData = 13,
    HideAdditionalTooltip = 14,
    HideTooltip = 15,
    RepairCost = 16,
    CreativeSlotLock = 17,
    EnchantmentGlintOverride = 18,
    IntangibleProjectile = 19,
    Food = 20,
    FireResistant = 21,
    Tool = 22,
    StoredEnchantments = 23,
    DyedColor = 24,
    MapColor = 25,
    MapID = 26,
    MapDecorations = 27,
    MapPostProcessing = 28,
    ChargedProjectiles = 29,
    BundleContents = 30,
    PotionContents = 31,
    SuspiciousStewEffects = 32,
    WritableBookContent = 33,
    WrittenBookContent = 34,
    Trim = 35,
    DebugStickState = 36,
    EntityData = 37,
    BucketEntityData = 38,
    BlockEntityData = 39,
    Instrument = 40,
    OminousBottleAmplifier = 41,
    JukeboxPlayable = 42,
    Recipes = 43,
    LodestoneTracker = 44,
    FireworkExplosion = 45,
    Fireworks = 46,
    Profile = 47,
    NoteBlockSound = 48,
    BannerPatterns = 49,
    BaseColor = 50,
    PotDecorations = 51,
    Container = 52,
    BlockState = 53,
    Bees = 54,
    Lock = 55,
    ContainerLoot = 56
};

using ComponentData = std::variant<
    std::vector<uint8_t> // Generic placeholder
>;

struct Component {
    ComponentType type;
    ComponentData data;
};

struct SlotData {
    size_t itemCount = 0;
    std::optional<size_t> itemId;
    std::optional<size_t> numCompToAdd;
    std::optional<size_t> numCompToRemove;
    std::optional<std::vector<Component>> compsToAdd;
    std::optional<std::vector<ComponentType>> compsToRemove;
};

Component parseComponent(const std::vector<uint8_t>& data, size_t& index);

#endif //SLOT_DATA_H
