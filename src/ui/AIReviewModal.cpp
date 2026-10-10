#include "AIReviewModal.h"
#include "../UI/UITheme.h"

AIReviewModal::AIReviewModal() : isOpen(false) {}

void AIReviewModal::init() {
    font.loadFromFile("assets/Jersey10-Regular.ttf");

    overlay.setSize(sf::Vector2f(1920.f, 1080.f));
    overlay.setFillColor(sf::Color(10, 4, 16, 210));

    modalBg.setSize(sf::Vector2f(1360.f, 760.f));
    modalBg.setOrigin(680.f, 380.f);
    modalBg.setPosition(960.f, 540.f);

    originalView.setSize(sf::Vector2f(580.f, 580.f));
    originalView.setPosition(330.f, 210.f);
    originalView.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    originalView.setOutlineThickness(1.5f);
    originalView.setOutlineColor(WisdomUI::Theme::SunsetPlum);

    resultView.setSize(sf::Vector2f(580.f, 580.f));
    resultView.setPosition(1010.f, 210.f);
    resultView.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    resultView.setOutlineThickness(1.5f);
    resultView.setOutlineColor(WisdomUI::Theme::SunsetAmber);

    acceptNewLayerBtn.setSize(sf::Vector2f(200.f, 40.f));
    acceptNewLayerBtn.setPosition(1010.f, 810.f);

    replaceLayerBtn.setSize(sf::Vector2f(180.f, 40.f));
    replaceLayerBtn.setPosition(1230.f, 810.f);

    newProjectBtn.setSize(sf::Vector2f(160.f, 40.f));
    newProjectBtn.setPosition(1430.f, 810.f);

    rejectBtn.setSize(sf::Vector2f(140.f, 40.f));
    rejectBtn.setPosition(330.f, 810.f);

    prevOptionBounds = sf::FloatRect(1186.f, 180.f, 34.f, 24.f);
    nextOptionBounds = sf::FloatRect(1380.f, 180.f, 34.f, 24.f);
}

void AIReviewModal::open(const sf::Image& originalImg, const std::vector<sf::Image>& resultOptions) {
    if (resultOptions.empty()) return;
    open(originalImg, resultOptions.front());
    options = resultOptions;
}

void AIReviewModal::showOption(int index) {
    int count = static_cast<int>(options.size());
    if (count < 2) return;
    optionIndex = (index % count + count) % count;
    resultSavedImage = options[optionIndex];
    resultTexture.loadFromImage(resultSavedImage);
    resultView.setTexture(&resultTexture, true);
}

void AIReviewModal::open(const sf::Image& originalImg, const sf::Image& resultImg) {
    isOpen = true;
    originalSavedImage = originalImg;
    resultSavedImage = resultImg;
    options.assign(1, resultImg);
    optionIndex = 0;
    nextFrameMode = false;

    if (originalImg.getSize().x > 0 && originalImg.getSize().y > 0) {
        originalTexture.loadFromImage(originalImg);
        originalView.setTexture(&originalTexture);
        // A fill colour tints the texture, so it has to be white for the preview to show true colours
        originalView.setFillColor(sf::Color::White);
    }
    if (resultImg.getSize().x > 0 && resultImg.getSize().y > 0) {
        resultTexture.loadFromImage(resultImg);
        resultView.setTexture(&resultTexture);
        resultView.setFillColor(sf::Color::White);
    }
}

void AIReviewModal::close() {
    isOpen = false;
    originalView.setTexture(nullptr);
    resultView.setTexture(nullptr);
    originalView.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    resultView.setFillColor(WisdomUI::Theme::SunsetDeepDark);
}

bool AIReviewModal::getIsOpen() const {
    return isOpen;
}

const sf::Image& AIReviewModal::getResultImage() const {
    return resultSavedImage;
}

const sf::Image& AIReviewModal::getOriginalImage() const {
    return originalSavedImage;
}

void AIReviewModal::draw(sf::RenderWindow& window) {
    if (!isOpen) return;

    window.draw(overlay);

    sf::FloatRect modalBounds(modalBg.getPosition().x - 680.f, modalBg.getPosition().y - 380.f, 1360.f, 760.f);
    WisdomUI::Theme::DrawSunsetPanel(window, modalBounds, 1.0f);

    WisdomUI::Theme::DrawCrispText(window, font, ":: REVIEW SUGGESTION ::", 18, 960.f, modalBounds.top + 28.f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 6, 20), true, true);

    sf::RectangleShape viewBg(originalView.getSize());
    viewBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    viewBg.setPosition(originalView.getPosition());
    window.draw(viewBg);
    viewBg.setPosition(resultView.getPosition());
    window.draw(viewBg);

    window.draw(originalView);
    window.draw(resultView);

    WisdomUI::Theme::DrawCrispText(window, font, nextFrameMode ? "THIS FRAME" : "SOURCE CANVAS", 12, 330.f + 290.f, 195.f, WisdomUI::Theme::TextSecondary, sf::Color::Transparent, true, true);
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    if (options.size() > 1) {
        std::string label = "OPTION " + std::to_string(optionIndex + 1) + " OF " + std::to_string(options.size()) + "   (LEFT / RIGHT)";
        WisdomUI::Theme::DrawCrispText(window, font, label, 12, 1010.f + 290.f, 195.f, WisdomUI::Theme::SunsetGold, sf::Color::Transparent, true, true);
        WisdomUI::Theme::DrawSunsetButton(window, prevOptionBounds, "<", font, 14, false, prevOptionBounds.contains(mPos), false, 1.0f);
        WisdomUI::Theme::DrawSunsetButton(window, nextOptionBounds, ">", font, 14, false, nextOptionBounds.contains(mPos), false, 1.0f);
    }
    else {
        WisdomUI::Theme::DrawCrispText(window, font, nextFrameMode ? "NEXT FRAME" : "GENERATED OUTPUT", 12, 1010.f + 290.f, 195.f, WisdomUI::Theme::SunsetGold, sf::Color::Transparent, true, true);
    }

    if (nextFrameMode) {
        WisdomUI::Theme::DrawSunsetButton(window, acceptNewLayerBtn.getGlobalBounds(), "Add As Next Frame", font, 12, false, acceptNewLayerBtn.getGlobalBounds().contains(mPos), true, 1.0f);
        WisdomUI::Theme::DrawSunsetButton(window, rejectBtn.getGlobalBounds(), "Discard Result", font, 12, false, rejectBtn.getGlobalBounds().contains(mPos), true, 1.0f);
        return;
    }

    WisdomUI::Theme::DrawSunsetButton(window, acceptNewLayerBtn.getGlobalBounds(), "Accept New Layer", font, 12, false, acceptNewLayerBtn.getGlobalBounds().contains(mPos), true, 1.0f);
    WisdomUI::Theme::DrawSunsetButton(window, replaceLayerBtn.getGlobalBounds(), "Update Layer", font, 12, false, replaceLayerBtn.getGlobalBounds().contains(mPos), false, 1.0f);
    WisdomUI::Theme::DrawSunsetButton(window, newProjectBtn.getGlobalBounds(), "New Project", font, 12, false, newProjectBtn.getGlobalBounds().contains(mPos), false, 1.0f);
    WisdomUI::Theme::DrawSunsetButton(window, rejectBtn.getGlobalBounds(), "Discard Result", font, 12, false, rejectBtn.getGlobalBounds().contains(mPos), true, 1.0f);
}

std::string AIReviewModal::handleEvent(const sf::Event& event, sf::Vector2f mousePos) {
    if (!isOpen) return "";
    if (event.type == sf::Event::KeyPressed && options.size() > 1) {
        if (event.key.code == sf::Keyboard::Left) showOption(optionIndex - 1);
        if (event.key.code == sf::Keyboard::Right) showOption(optionIndex + 1);
    }
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (options.size() > 1 && prevOptionBounds.contains(mousePos)) showOption(optionIndex - 1);
        if (options.size() > 1 && nextOptionBounds.contains(mousePos)) showOption(optionIndex + 1);
        if (acceptNewLayerBtn.getGlobalBounds().contains(mousePos)) {
            close();
            return nextFrameMode ? "accept_frame" : "accept_new";
        }
        if (nextFrameMode) {
            if (rejectBtn.getGlobalBounds().contains(mousePos)) {
                close();
                return "reject";
            }
            return "consumed";
        }
        if (replaceLayerBtn.getGlobalBounds().contains(mousePos)) {
            close();
            return "accept_replace";
        }
        if (newProjectBtn.getGlobalBounds().contains(mousePos)) {
            close();
            return "accept_project";
        }
        if (rejectBtn.getGlobalBounds().contains(mousePos)) {
            close();
            return "reject";
        }
    }
    return "consumed";
}