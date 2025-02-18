#include "GameScene.h"
#include <cassert>
#include <CCUserDefault.h>
#include "PluginFacebook/PluginFacebook.h"

USING_NS_CC;


Scene* GameScene::createScene()
{
    auto scene = Scene::createWithPhysics();
    scene->getPhysicsWorld()->setGravity(Vec2(0, -500));
    auto layer = GameScene::create();
    layer->setPhysicsWorld(scene->getPhysicsWorld());
    scene->addChild(layer);
    return scene;
}

bool GameScene::init()
{
// make background white
    if ( !LayerColor::initWithColor(Color4B::WHITE)) {
        return false;
    }
    cocos2d::Size visibleSize = Director::getInstance()->getVisibleSize();
    auto center = cocos2d::Point(visibleSize.width / 2, visibleSize.height / 2);
    
// add label
    label = Label::createWithTTF("Drag the number cell into board!", "Naughty Cartoons.ttf", 40);
    label->setPosition(cocos2d::Point(visibleSize.width / 2,
                                      visibleSize.height - 100));
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    
// initalize some class member
    movingSprite = nullptr;
    int diff = UserDefault::getInstance()->getIntegerForKey("difficulty");
    game.regenerate(diff);
    CCLOG("game scene %d", diff);
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            state[i][j] = game.startStatus[i][j];
        }
    }
    musicPlaying = true;
    
// menu in game scene
    MenuItemFont::setFontSize(50);
    //auto menu_item_1 = MenuItemFont::create("Answer", CC_CALLBACK_1(GameScene::showAnswer, this));
    auto menu_item_1 = MenuItemFont::create("Answer", CC_CALLBACK_0(GameScene::showAnswer, this));

    menu_item_1->setColor(Color3B::BLACK);
    menu_item_1->setPosition(visibleSize.width - 150, visibleSize.height -100 );
    
    auto menu_item_2 = MenuItemFont::create("Share", CC_CALLBACK_0(GameScene::share, this));
    menu_item_2->setColor(Color3B::BLACK);
    menu_item_2->setPosition(visibleSize.width - 150, visibleSize.height -200 );
    
    auto menu = Menu::create(menu_item_1, menu_item_2, nullptr);
    menu->setPosition(cocos2d::Point(0, 0));
    this->addChild(menu);
// pre load music effect
    CocosDenshion::SimpleAudioEngine::getInstance()->preloadEffect("Droplet.wav");
    CocosDenshion::SimpleAudioEngine::getInstance()->preloadEffect("wrong.wav");
    
// add game board
    board = Sprite::create("board.png");
    board->setPosition(center);
    this->addChild(board);
    
// calculate sizes and positions
    auto boardSize = board->getContentSize();
    auto boardPosition = board->getPosition();
    boardTopLeft.x = boardPosition.x - boardSize.width / 2;
    boardTopLeft.y = boardPosition.y + boardSize.height / 2;
    cellLength = boardSize.height / 9;

// add number in fix cell
    
    emptyCellinBoardCount = 0;
    
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            int temp = game.startStatus[i][j];
            if (temp != 0) {
                auto fixedCell = Label::createWithTTF(std::to_string(temp), "squarefont.ttf", 50);
                fixedCell->setColor(Color3B::BLACK);
                fixedCell->setAnchorPoint(Vec2(0.5, 0.5));
                fixedCell->setPosition(boardTopLeft.x + ( j + 0.5 ) * cellLength,
                                       boardTopLeft.y - ( i + 0.5 ) * cellLength);
                this->addChild(fixedCell);
            } else {
                emptyCellinBoardCount++;
            }
        }
    }
    
// create candidate number cell
    for (int i = 1; i <= 9 ; i++) {
        numberSprite *numberCell = numberSprite::create(to_string(i) + ".png");
        numberCell->num = i;
        numberCell->inBoard = false;
        initPosition.push_back(cocos2d::Point(boardTopLeft.x / 2,
                                              visibleSize.height - visibleSize.height / 10 * i));
        numberCell->setPosition(initPosition[i-1]);
        this->addChild(numberCell);
        moveAbleCell.push_back(numberCell);
    }
    
// set up touch event listener using lambda expression, c++11 feature
    auto touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);
    touchListener->onTouchBegan = [&](Touch* touch, Event* event) -> bool {
        cocos2d::Point touchPosition = touch->getLocation();
        selectSpriteForTouch(touchPosition);
        if (movingSprite) {
            auto action = ScaleTo::create(0.1, 1.3);
            movingSprite->runAction(action);
            if (movingSprite->inBoard == true) {
                state[movingSprite->currentRow][movingSprite->currentColumn] = 0;
                emptyCellinBoardCount++;
            }
        }
        return true;
    };
    touchListener->onTouchMoved = [&] (Touch* touch, Event* event) -> void {
        if (movingSprite) {
            cocos2d::Point touchLocation = touch->getLocation();
            cocos2d::Point oldTouchLocation = touch->getPreviousLocation();
            Vec2 translation = touchLocation - oldTouchLocation;
            this->updateMovingSpritePosition(translation);
        }
    };
    touchListener->onTouchEnded = [&] (Touch* touch, Event* event) -> void {
        if (movingSprite) {
            cocos2d::Point touchLocation = touch->getLocation();
            adjustPosition(touchLocation);
            auto action = ScaleTo::create(0.1, 1.0);
            movingSprite->runAction(action);
            movingSprite = nullptr;
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);
    

// set mute button
    auto mute = cocos2d::ui::Button::create("mute.png");
    mute->setPosition( cocos2d::Point(visibleSize.width - 100, 100) );
    
    mute->addTouchEventListener([=](Ref* sender, cocos2d::ui::Widget::TouchEventType type){
        
        if (type == ui::Widget::TouchEventType::BEGAN) {
            if (musicPlaying) {
                CocosDenshion::SimpleAudioEngine::getInstance()->pauseBackgroundMusic();
                mute->loadTextureNormal("unmute.png");
                musicPlaying = false;
            } else {
                CocosDenshion::SimpleAudioEngine::getInstance()->resumeBackgroundMusic();
                mute->loadTextureNormal("mute.png");
                musicPlaying = true;
            }
        }
        
    });
    this->addChild(mute);
    
// reset the board
    
    replay = cocos2d::ui::Button::create("replay.png");
    replay->setPosition( cocos2d::Point(visibleSize.width - 100, 250) );
    
    
    replay->addTouchEventListener([&](Ref* sender, cocos2d::ui::Widget::TouchEventType type){
        if (type == ui::Widget::TouchEventType::BEGAN) {
            CocosDenshion::SimpleAudioEngine::getInstance()->playEffect("close1.wav");
            resetBoard();
        }
    });
    this->addChild(replay);

// go back to main menuscene
    
    goBackToMainMenu = cocos2d::ui::Button::create("back.png");
    goBackToMainMenu->setPosition( cocos2d::Point(visibleSize.width - 100, 400) );
    
    
    goBackToMainMenu->addTouchEventListener([&](Ref* sender, cocos2d::ui::Widget::TouchEventType type){
        if (type == ui::Widget::TouchEventType::BEGAN) {
            CocosDenshion::SimpleAudioEngine::getInstance()->playEffect("click.wav");
            Director::getInstance()->popSceneWithTransition<TransitionSlideInR>(TRANSITION_TIME);
        }
    });
    this->addChild(goBackToMainMenu);
    
    return true;
}

// for onTouchBegan method, set movingSprite
void GameScene::selectSpriteForTouch(cocos2d::Point p) {
    for (int i = 0; i < moveAbleCell.size(); i++) {
        if (moveAbleCell[i]->getBoundingBox().containsPoint(p)) {
            movingSprite = moveAbleCell[i];
        }
    }
}

// for onTouchMoved
void GameScene::updateMovingSpritePosition(Vec2 p) {
    cocos2d::Point oldPosition = movingSprite->getPosition();
    movingSprite->setPosition(oldPosition + p);
}

// for onTouchEnd
void GameScene::adjustPosition(cocos2d::Point locationBeforeAdjust) {
    // move into boarrd cell
    bool dragTheCellInBoardAndTheCellIsNotEmpty = false;
    
    // drag number into board
    if (board->getBoundingBox().containsPoint(locationBeforeAdjust)) {
        int x = locationBeforeAdjust.x;
        int y = locationBeforeAdjust.y;
        int top_left_x = boardTopLeft.x;
        int top_left_y = boardTopLeft.y;
        int row_count_x = ( x - top_left_x ) / cellLength;
        int row_count_y = ( top_left_y - y ) / cellLength;
        
        //cell is empty
        if (state[row_count_y][row_count_x] == 0) {
            int new_x = ( row_count_x + 0.5 ) * cellLength + top_left_x;
            int new_y = top_left_y - ( row_count_y + 0.5 ) * cellLength;
            
            state[row_count_y][row_count_x] = movingSprite->num;
            if (movingSprite->inBoard == false) {           // drag number cell from left into board
                addNewNumberCell(movingSprite->num);
            }
            movingSprite->inBoard = true;
            movingSprite->currentRow = row_count_y;
            movingSprite->currentColumn = row_count_x;
            emptyCellinBoardCount--;
            if (emptyCellinBoardCount == 0) {
                if (checkGameBoard()) {
                    MessageBox("", "success");
                    label->setString("success");
                } else {
                    MessageBox("", "some thing is incorrect");
                    CocosDenshion::SimpleAudioEngine::getInstance()->playEffect("wrong.wav");
                    label->setString("some thing is not correct");
                }
            }
            auto action = MoveTo::create(0.1, cocos2d::Point(new_x, new_y));
            movingSprite->runAction(action);
            CocosDenshion::SimpleAudioEngine::getInstance()->playEffect("Droplet.wav");
            return;
        }
        // cell is not empty
        dragTheCellInBoardAndTheCellIsNotEmpty = true;    // put cell in board and the block is not empty
    }
    
// should not handle emptyCellCount here, it was handled in onTouchBegin
// !dragTheCellInBoardAndTheCellIsNotEmpty means:
// successfully drag number from cell out of board
    
    int num = movingSprite->num;
    movingSprite->inBoard = false;
    movingSprite->currentColumn = movingSprite->currentRow = -1;
    //if (!dragTheCellInBoardAndTheCellIsNotEmpty) {
    //    for (auto it = moveAbleCell.begin(); it != moveAbleCell.end(); it++) {
    //        if (*it == movingSprite) {
    //            moveAbleCell.erase(it);
    //            break;
    //        }
    //    }
    //}
    auto action = MoveTo::create(0.2, initPosition[num-1]);
    movingSprite->runAction(action);
}

void GameScene::addNewNumberCell(int num) {
    numberSprite *newNumCell = numberSprite::create(to_string(num) + ".png");
    newNumCell->setPosition(initPosition[num-1]);
    newNumCell->num = num;
    newNumCell->inBoard = false;
    newNumCell->currentRow = newNumCell->currentColumn = -1;
    RotateBy * rotLeft = RotateBy::create(0.1, -14.0);
    RotateBy * rotCenter = RotateBy::create(0.1, 0.0);
    RotateBy * rotRight = RotateBy::create(0.1, 14.0);
    Sequence * rotSeq = Sequence::create(rotLeft, rotCenter, rotRight, rotCenter, NULL);
    newNumCell->runAction(rotSeq);
    moveAbleCell.push_back(newNumCell);
    this->addChild(newNumCell);
}

bool GameScene::checkGameBoard() {
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (game.endStatus[i][j] != state[i][j]) {
                return false;
            }
        }
    }
    return true;
}

void GameScene::resetBoard() {
    //CCLOG("%lu a", moveAbleCell.size());
    for (auto it = moveAbleCell.begin(); it != moveAbleCell.end(); ) {
        if ((*it)->inBoard) {
            auto drop = PhysicsBody::createBox( moveAbleCell[0]->getContentSize(), PhysicsMaterial( 1, 0.5, 0 ) );
            (*it)->setPhysicsBody(drop);
            (*it)->inBoard = false;
            state[(*it)->currentRow][(*it)->currentColumn] = 0;
            moveAbleCell.erase(it);
        } else {
            ++it;
        }
    }
    //assert(moveAbleCell.size() == 9);
// clear state
    emptyCellinBoardCount = 0;
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (game.startStatus[i][j] == 0) {
                state[i][j] = 0;
                emptyCellinBoardCount++;
            }
        }
    }
    CCLOG("empty cell %d", emptyCellinBoardCount);
    //CCLOG("%lu b", moveAbleCell.size());
}

// no need to handle emptyCellinBoardCount in this function, it is handled in resetBoard

void GameScene::showAnswer() {
    
    CCLOG("show answer %d", answerShowing);
    
    if (!answerShowing) {
        auto boardSize = board->getContentSize();
        auto boardPosition = board->getPosition();
        boardTopLeft.x = boardPosition.x - boardSize.width / 2;
        boardTopLeft.y = boardPosition.y + boardSize.height / 2;
        cellLength = boardSize.height / 9;
        
        resetBoard();
        
        for (int i = 0; i < 9; i++) {
            for (int j = 0; j < 9; j++) {
                int temp = game.startStatus[i][j];
                if (temp == 0) {
                    auto fixedCell = Label::createWithTTF(std::to_string(game.endStatus[i][j]), "squarefont.ttf", 50);
                    fixedCell->setColor(Color3B::GRAY);
                    fixedCell->setAnchorPoint(Vec2(0.5, 0.5));
                    fixedCell->setPosition(boardTopLeft.x + ( j + 0.5 ) * cellLength,
                                           boardTopLeft.y - ( i + 0.5 ) * cellLength);
                    ans.insert(fixedCell);
                    this->addChild(fixedCell);
                }
            }
        }
        answerShowing = true;
    } else {
        for (auto it = ans.begin(); it != ans.end(); ++it) {
            (*it)->removeFromParentAndCleanup(true);
        }
        ans.clear();
        answerShowing = false;
    }
}

void GameScene::share() {
    sdkbox::PluginFacebook::login();
    //sdkbox::PluginFacebook::requestReadPermissions({sdkbox::FB_PERM_READ_PUBLIC_PROFILE, sdkbox::FB_PERM_READ_USER_FRIENDS});
    sdkbox::PluginFacebook::requestPublishPermissions({sdkbox::FB_PERM_PUBLISH_POST});
    sdkbox::FBShareInfo info;
    info.type  = sdkbox::FB_LINK;
    info.link  = "http://www.cocos2d-x.org";
    info.title = "hehe";
    info.text  = "haha";
    info.image = "http://cocos2d-x.org/images/logo.png";
    sdkbox::PluginFacebook::share(info);

    CCLOG("share");
}
