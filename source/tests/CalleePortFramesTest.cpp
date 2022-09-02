/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gmock/gmock.h>

#include <mariana-trench/Access.h>
#include <mariana-trench/CalleePortFrames.h>
#include <mariana-trench/Redex.h>
#include <mariana-trench/tests/Test.h>

namespace marianatrench {

class CalleePortFramesTest : public test::Test {};

TEST_F(CalleePortFramesTest, Add) {
  auto context = test::make_empty_context();

  Scope scope;
  auto* one = context.methods->create(
      redex::create_void_method(scope, "LClass;", "one"));
  auto* two = context.methods->create(
      redex::create_void_method(scope, "LOther;", "two"));

  auto* source_kind_one = context.kinds->get("TestSourceOne");
  auto* source_kind_two = context.kinds->get("TestSourceTwo");
  auto* feature_one = context.features->get("FeatureOne");
  auto* feature_two = context.features->get("FeatureTwo");
  auto* user_feature_one = context.features->get("UserFeatureOne");

  CalleePortFrames frames;
  EXPECT_TRUE(frames.is_bottom());
  EXPECT_TRUE(frames.empty());

  frames.add(test::make_taint_config(
      source_kind_one,
      test::FrameProperties{
          .callee_port = AccessPath(Root(Root::Kind::Return)),
          .origins = MethodSet{one},
          .inferred_features = FeatureMayAlwaysSet{feature_one}}));
  EXPECT_FALSE(frames.is_bottom());
  EXPECT_EQ(frames.callee_port(), AccessPath(Root(Root::Kind::Return)));
  EXPECT_EQ(
      frames,
      CalleePortFrames(
          /* local_positions */ {},
          {test::make_taint_config(
              source_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Return)),
                  .origins = MethodSet{one},
                  .inferred_features = FeatureMayAlwaysSet{feature_one}})}));

  // Add frame with the same kind
  frames.add(test::make_taint_config(
      source_kind_one,
      test::FrameProperties{
          .callee_port = AccessPath(Root(Root::Kind::Return)),
          .origins = MethodSet{two},
          .inferred_features = FeatureMayAlwaysSet{feature_two},
          .user_features = FeatureSet{user_feature_one}}));
  EXPECT_EQ(
      frames,
      CalleePortFrames(
          /* local_positions */ {},
          {test::make_taint_config(
              source_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Return)),
                  .origins = MethodSet{one, two},
                  .inferred_features =
                      FeatureMayAlwaysSet::make_may({feature_one, feature_two}),
                  .user_features = FeatureSet{user_feature_one}})}));

  // Add frame with a different kind
  frames.add(test::make_taint_config(
      source_kind_two,
      test::FrameProperties{
          .callee_port = AccessPath(Root(Root::Kind::Return)),
          .origins = MethodSet{two},
          .inferred_features = FeatureMayAlwaysSet{feature_two}}));
  EXPECT_EQ(
      frames,
      (CalleePortFrames(
          /* local_positions */ {},
          {test::make_taint_config(
               source_kind_one,
               test::FrameProperties{
                   .callee_port = AccessPath(Root(Root::Kind::Return)),
                   .origins = MethodSet{one, two},
                   .inferred_features = FeatureMayAlwaysSet::make_may(
                       {feature_one, feature_two}),
                   .user_features = FeatureSet{user_feature_one}}),
           test::make_taint_config(
               source_kind_two,
               test::FrameProperties{
                   .callee_port = AccessPath(Root(Root::Kind::Return)),
                   .origins = MethodSet{two},
                   .inferred_features = FeatureMayAlwaysSet{feature_two}})})));

  // Additional test for when callee_port == default value selected by
  // constructor in the implementation.
  frames = CalleePortFrames();
  frames.add(test::make_taint_config(source_kind_one, test::FrameProperties{}));
  EXPECT_EQ(frames.callee_port(), AccessPath(Root(Root::Kind::Leaf)));
}

TEST_F(CalleePortFramesTest, Leq) {
  auto context = test::make_empty_context();

  Scope scope;
  auto* one =
      context.methods->create(redex::create_void_method(scope, "LOne;", "one"));

  auto* test_kind_one = context.kinds->get("TestSinkOne");
  auto* test_kind_two = context.kinds->get("TestSinkTwo");

  // Comparison to bottom
  EXPECT_TRUE(CalleePortFrames::bottom().leq(CalleePortFrames::bottom()));
  EXPECT_TRUE(CalleePortFrames::bottom().leq(CalleePortFrames(
      /* local_positions */ {},
      {test::make_taint_config(test_kind_one, test::FrameProperties{})})));
  EXPECT_FALSE(
      (CalleePortFrames(
           /* local_positions */ {},
           {test::make_taint_config(test_kind_one, test::FrameProperties{})}))
          .leq(CalleePortFrames::bottom()));
  EXPECT_FALSE(
      (CalleePortFrames(
           /* local_positions */ {},
           {test::make_taint_config(
               test_kind_one,
               test::FrameProperties{
                   .callee_port = AccessPath(Root(Root::Kind::Argument, 0))})}))
          .leq(CalleePortFrames::bottom()));

  // Comparison to self
  EXPECT_TRUE(
      (CalleePortFrames(
           /* local_positions */ {},
           {test::make_taint_config(
               test_kind_one,
               test::FrameProperties{
                   .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                   .callee = one,
                   .distance = 1,
                   .origins = MethodSet{one}})}))
          .leq(CalleePortFrames(
              /* local_positions */ {},
              {test::make_taint_config(
                  test_kind_one,
                  test::FrameProperties{
                      .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                      .callee = one,
                      .distance = 1,
                      .origins = MethodSet{one}})})));

  // Different kinds
  EXPECT_TRUE(
      (CalleePortFrames(
           /* local_positions */ {},
           {test::make_taint_config(
               test_kind_one,
               test::FrameProperties{
                   .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                   .callee = one,
                   .distance = 1,
                   .origins = MethodSet{one}})}))
          .leq(CalleePortFrames(
              /* local_positions */ {},
              {
                  test::make_taint_config(
                      test_kind_one,
                      test::FrameProperties{
                          .callee_port =
                              AccessPath(Root(Root::Kind::Argument, 0)),
                          .callee = one,
                          .distance = 1,
                          .origins = MethodSet{one}}),
                  test::make_taint_config(
                      test_kind_two,
                      test::FrameProperties{
                          .callee_port =
                              AccessPath(Root(Root::Kind::Argument, 0)),
                          .callee = one,
                          .distance = 1,
                          .origins = MethodSet{one}}),
              })));
  EXPECT_FALSE(
      (CalleePortFrames(
           /* local_positions */ {},
           {test::make_taint_config(
                test_kind_one,
                test::FrameProperties{
                    .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                    .callee = one,
                    .distance = 1,
                    .origins = MethodSet{one}}),
            test::make_taint_config(
                test_kind_two,
                test::FrameProperties{
                    .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                    .callee = one,
                    .distance = 1,
                    .origins = MethodSet{one}})}))
          .leq(CalleePortFrames(
              /* local_positions */ {},
              {test::make_taint_config(
                  test_kind_one,
                  test::FrameProperties{
                      .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                      .callee = one,
                      .distance = 1,
                      .origins = MethodSet{one}})})));
}

TEST_F(CalleePortFramesTest, ArtificialSourceLeq) {
  auto context = test::make_empty_context();

  // For artificial sources, compare the common prefix of callee ports.
  EXPECT_TRUE(CalleePortFrames(
                  /* local_positions */ {},
                  {test::make_taint_config(
                      Kinds::artificial_source(),
                      test::FrameProperties{
                          .callee_port = AccessPath(
                              Root(Root::Kind::Argument, 0),
                              Path{DexString::make_string("x")})})})
                  .leq(CalleePortFrames(
                      /* local_positions */ {},
                      {test::make_taint_config(
                          Kinds::artificial_source(),
                          test::FrameProperties{
                              .callee_port = AccessPath(
                                  Root(Root::Kind::Argument, 0))})})));
  EXPECT_FALSE(
      CalleePortFrames(
          /* local_positions */ {},
          {test::make_taint_config(
              Kinds::artificial_source(),
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0))})})
          .leq(CalleePortFrames(
              /* local_positions */ {},
              {test::make_taint_config(
                  Kinds::artificial_source(),
                  test::FrameProperties{
                      .callee_port = AccessPath(
                          Root(Root::Kind::Argument, 0),
                          Path{DexString::make_string("x")})})})));
}

TEST_F(CalleePortFramesTest, Equals) {
  auto context = test::make_empty_context();

  auto* test_kind_one = context.kinds->get("TestSinkOne");
  auto* test_kind_two = context.kinds->get("TestSinkTwo");

  // Comparison to bottom
  EXPECT_TRUE(CalleePortFrames::bottom().equals(CalleePortFrames::bottom()));
  EXPECT_FALSE(CalleePortFrames::bottom().equals(CalleePortFrames(
      /* local_positions */ {},
      {test::make_taint_config(test_kind_one, test::FrameProperties{})})));
  EXPECT_FALSE(
      (CalleePortFrames(
           /* local_positions */ {},
           {test::make_taint_config(test_kind_one, test::FrameProperties{})}))
          .equals(CalleePortFrames::bottom()));

  // Comparison to self
  EXPECT_TRUE(
      (CalleePortFrames(
           /* local_positions */ {},
           {test::make_taint_config(test_kind_one, test::FrameProperties{})}))
          .equals(CalleePortFrames(
              /* local_positions */ {},
              {test::make_taint_config(
                  test_kind_one, test::FrameProperties{})})));

  // Different kinds
  EXPECT_FALSE(
      (CalleePortFrames(
           /* local_positions */ {},
           {test::make_taint_config(test_kind_one, test::FrameProperties{})}))
          .equals(CalleePortFrames(
              /* local_positions */ {},
              {test::make_taint_config(
                  test_kind_two, test::FrameProperties{})})));
}

TEST_F(CalleePortFramesTest, JoinWith) {
  auto context = test::make_empty_context();

  Scope scope;
  auto* one =
      context.methods->create(redex::create_void_method(scope, "LOne;", "one"));

  auto* test_kind_one = context.kinds->get("TestSinkOne");
  auto* test_kind_two = context.kinds->get("TestSinkTwo");

  // Join with bottom.
  EXPECT_EQ(
      CalleePortFrames::bottom().join(CalleePortFrames(
          /* local_positions */ {},
          {test::make_taint_config(test_kind_one, test::FrameProperties{})})),
      CalleePortFrames(
          /* local_positions */ {},
          {test::make_taint_config(test_kind_one, test::FrameProperties{})}));

  EXPECT_EQ(
      (CalleePortFrames(
           /* local_positions */ {},
           {test::make_taint_config(test_kind_one, test::FrameProperties{})}))
          .join(CalleePortFrames::bottom()),
      CalleePortFrames(
          /* local_positions */ {},
          {test::make_taint_config(test_kind_one, test::FrameProperties{})}));

  // Additional test to verify that joining with bottom adopts the new port
  // and not the default "leaf" port.
  auto frames =
      (CalleePortFrames(
           /* local_positions */ {},
           {test::make_taint_config(
               test_kind_one,
               test::FrameProperties{
                   .callee_port = AccessPath(Root(Root::Kind::Return))})}))
          .join(CalleePortFrames::bottom());
  EXPECT_EQ(
      frames,
      CalleePortFrames(
          /* local_positions */ {},
          {test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Return))})}));
  EXPECT_EQ(frames.callee_port(), AccessPath(Root(Root::Kind::Return)));

  frames = CalleePortFrames::bottom().join(CalleePortFrames(
      /* local_positions */ {},
      {test::make_taint_config(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Return))})}));
  EXPECT_EQ(
      frames,
      CalleePortFrames(
          /* local_positions */ {},
          {test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Return))})}));
  EXPECT_EQ(frames.callee_port(), AccessPath(Root(Root::Kind::Return)));

  // Join different kinds
  frames = CalleePortFrames(
      /* local_positions */ {},
      {test::make_taint_config(test_kind_one, test::FrameProperties{})});
  frames.join_with(CalleePortFrames(
      /* local_positions */ {},
      {test::make_taint_config(test_kind_two, test::FrameProperties{})}));
  EXPECT_EQ(
      frames,
      (CalleePortFrames(
          /* local_positions */ {},
          {test::make_taint_config(test_kind_one, test::FrameProperties{}),
           test::make_taint_config(test_kind_two, test::FrameProperties{})})));

  // Join same kind
  auto frame_one = test::make_taint_config(
      test_kind_one,
      test::FrameProperties{
          .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
          .callee = one,
          .distance = 1,
          .origins = MethodSet{one}});
  auto frame_two = test::make_taint_config(
      test_kind_one,
      test::FrameProperties{
          .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
          .callee = one,
          .distance = 2,
          .origins = MethodSet{one}});
  frames = CalleePortFrames(/* local_positions */ {}, {frame_one});
  frames.join_with(CalleePortFrames(/* local_positions */ {}, {frame_two}));
  EXPECT_EQ(frames, (CalleePortFrames(/* local_positions */ {}, {frame_one})));
}

TEST_F(CalleePortFramesTest, ArtificialSourceJoinWith) {
  // Join different ports with same prefix for artificial kinds.
  // Ports should be collapsed to the common prefix.
  auto frames = CalleePortFrames(
      /* local_positions */ {},
      {test::make_taint_config(
          Kinds::artificial_source(),
          test::FrameProperties{
              .callee_port = AccessPath(
                  Root(Root::Kind::Argument, 0),
                  Path{DexString::make_string("x")})})});
  frames.join_with(CalleePortFrames(
      /* local_positions */ {},
      {test::make_taint_config(
          Kinds::artificial_source(),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0))})}));
  EXPECT_EQ(
      frames,
      CalleePortFrames(
          /* local_positions */ {},
          {test::make_taint_config(
              Kinds::artificial_source(),
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0))})}));
}

TEST_F(CalleePortFramesTest, Difference) {
  auto context = test::make_empty_context();

  Scope scope;
  auto* one =
      context.methods->create(redex::create_void_method(scope, "LOne;", "one"));
  auto* two =
      context.methods->create(redex::create_void_method(scope, "LTwo;", "two"));
  auto* three = context.methods->create(
      redex::create_void_method(scope, "LThree;", "three"));

  auto* test_kind_one = context.kinds->get("TestSinkOne");
  auto* test_kind_two = context.kinds->get("TestSinkTwo");
  auto* feature_one = context.features->get("FeatureOne");
  auto* feature_two = context.features->get("FeatureTwo");
  auto* user_feature_one = context.features->get("UserFeatureOne");
  auto* user_feature_two = context.features->get("UserFeatureTwo");

  CalleePortFrames frames, initial_frames;

  // Tests with empty left hand side.
  frames.difference_with(CalleePortFrames{});
  EXPECT_TRUE(frames.is_bottom());

  frames.difference_with(CalleePortFrames(
      /* local_positions */ {},
      {
          test::make_taint_config(test_kind_one, test::FrameProperties{}),
      }));
  EXPECT_TRUE(frames.is_bottom());

  initial_frames = CalleePortFrames(
      /* local_positions */ {},
      {
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = one,
                  .distance = 1,
                  .origins = MethodSet{one},
                  .inferred_features = FeatureMayAlwaysSet{feature_one},
                  .user_features = FeatureSet{user_feature_one}}),
      });

  frames = initial_frames;
  frames.difference_with(CalleePortFrames::bottom());
  EXPECT_EQ(frames, initial_frames);

  frames = initial_frames;
  frames.difference_with(CalleePortFrames(
      /* local_positions */ {},
      {
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = one,
                  .distance = 1,
                  .origins = MethodSet{one},
                  .inferred_features = FeatureMayAlwaysSet{feature_one},
                  .user_features = FeatureSet{user_feature_one}}),
      }));
  EXPECT_TRUE(frames.is_bottom());

  // Left hand side is bigger than right hand side.
  frames = initial_frames;
  frames.difference_with(CalleePortFrames(
      /* local_positions */ {},
      {
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = one,
                  .distance = 1,
                  .origins = MethodSet{one}}),
      }));
  EXPECT_EQ(frames, initial_frames);

  // Left hand side and right hand side have different inferred features.
  frames = initial_frames;
  frames.difference_with(CalleePortFrames(
      /* local_positions */ {},
      {
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = one,
                  .distance = 1,
                  .origins = MethodSet{one},
                  .inferred_features = FeatureMayAlwaysSet{feature_two},
                  .user_features = FeatureSet{user_feature_one}}),
      }));
  EXPECT_EQ(frames, initial_frames);

  // Left hand side and right hand side have different user features.
  frames = initial_frames;
  frames.difference_with(CalleePortFrames(
      /* local_positions */ {},
      {
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = one,
                  .distance = 1,
                  .origins = MethodSet{one},
                  .inferred_features = FeatureMayAlwaysSet{feature_one},
                  .user_features = FeatureSet{user_feature_two}}),
      }));
  EXPECT_EQ(frames, initial_frames);

  // Left hand side is smaller than right hand side (with one kind).
  frames = CalleePortFrames(
      /* local_positions */ {},
      {
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = one,
                  .distance = 1,
                  .origins = MethodSet{one},
                  .inferred_features = FeatureMayAlwaysSet{feature_one},
                  .user_features = FeatureSet{user_feature_one}}),
      });
  frames.difference_with(CalleePortFrames(
      /* local_positions */ {},
      {
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = one,
                  .distance = 1,
                  .origins = MethodSet{one},
                  .inferred_features = FeatureMayAlwaysSet{feature_one},
                  .user_features = FeatureSet{user_feature_one}}),
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = two,
                  .distance = 1,
                  .origins = MethodSet{two},
                  .inferred_features = FeatureMayAlwaysSet{feature_two},
                  .user_features = FeatureSet{user_feature_two}}),
      }));
  EXPECT_TRUE(frames.is_bottom());

  // Left hand side has more kinds than right hand side.
  frames = CalleePortFrames(
      /* local_positions */ {},
      {
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = one,
                  .distance = 1,
                  .origins = MethodSet{one}}),
          test::make_taint_config(
              test_kind_two,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = one,
                  .distance = 1,
                  .origins = MethodSet{one}}),
      });
  frames.difference_with(CalleePortFrames(
      /* local_positions */ {},
      {test::make_taint_config(
          test_kind_one,
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
              .callee = one,
              .distance = 1,
              .origins = MethodSet{one}})}));
  EXPECT_EQ(
      frames,
      (CalleePortFrames(
          /* local_positions */ {},
          {
              test::make_taint_config(
                  test_kind_two,
                  test::FrameProperties{
                      .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                      .callee = one,
                      .distance = 1,
                      .origins = MethodSet{one}}),
          })));

  // Left hand side is smaller for one kind, and larger for another.
  frames = CalleePortFrames(
      /* local_positions */ {},
      {
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = one,
                  .distance = 1,
                  .origins = MethodSet{one}}),
          test::make_taint_config(
              test_kind_two,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = two,
                  .distance = 1,
                  .origins = MethodSet{two}}),
          test::make_taint_config(
              test_kind_two,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = three,
                  .distance = 1,
                  .origins = MethodSet{three}}),
      });
  frames.difference_with(CalleePortFrames(
      /* local_positions */ {},
      {test::make_taint_config(
           test_kind_one,
           test::FrameProperties{
               .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
               .callee = one,
               .distance = 1,
               .origins = MethodSet{one}}),
       test::make_taint_config(
           test_kind_one,
           test::FrameProperties{
               .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
               .callee = two,
               .distance = 1,
               .origins = MethodSet{two}}),
       test::make_taint_config(
           test_kind_two,
           test::FrameProperties{
               .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
               .callee = two,
               .distance = 1,
               .origins = MethodSet{two}})}));
  EXPECT_EQ(
      frames,
      (CalleePortFrames(
          /* local_positions */ {},
          {test::make_taint_config(
              test_kind_two,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = three,
                  .distance = 1,
                  .origins = MethodSet{three}})})));

  // Left hand side larger than right hand side for specific frames.
  frames = CalleePortFrames(
      /* local_positions */ {},
      {
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = one,
                  .distance = 1,
                  .origins = MethodSet{one, two}}),
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = two,
                  .distance = 1,
                  .origins = MethodSet{two}}),
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = three,
                  .distance = 1,
                  .origins = MethodSet{one, three}}),
      });
  frames.difference_with(CalleePortFrames(
      /* local_positions */ {},
      {
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = one,
                  .distance = 1,
                  .origins = MethodSet{one}}),
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = three,
                  .distance = 1,
                  .origins = MethodSet{one, two, three}}),
      }));
  EXPECT_EQ(
      frames,
      (CalleePortFrames(
          /* local_positions */ {},
          {
              test::make_taint_config(
                  test_kind_one,
                  test::FrameProperties{
                      .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                      .callee = one,
                      .distance = 1,
                      .origins = MethodSet{one, two}}),
              test::make_taint_config(
                  test_kind_one,
                  test::FrameProperties{
                      .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                      .callee = two,
                      .distance = 1,
                      .origins = MethodSet{two}}),
          })));
}

TEST_F(CalleePortFramesTest, DifferenceLocalPositions) {
  auto context = test::make_empty_context();

  auto* test_kind_one = context.kinds->get("TestSinkOne");
  auto* test_kind_two = context.kinds->get("TestSinkTwo");
  auto* test_position_one = context.positions->get(std::nullopt, 1);

  // Empty left hand side.
  auto frames = CalleePortFrames::bottom();
  frames.difference_with(CalleePortFrames(
      LocalPositionSet{test_position_one},
      {
          test::make_taint_config(test_kind_one, test::FrameProperties{}),
      }));
  EXPECT_TRUE(frames.is_bottom());

  // lhs.local_positions <= rhs.local_positions
  // lhs.frames <= rhs.frames
  frames = CalleePortFrames(
      /* local_positions */ {},
      {test::make_taint_config(test_kind_one, test::FrameProperties{})});
  frames.difference_with(CalleePortFrames(
      LocalPositionSet{test_position_one},
      {test::make_taint_config(test_kind_one, test::FrameProperties{}),
       test::make_taint_config(test_kind_two, test::FrameProperties{})}));
  EXPECT_TRUE(frames.is_bottom());

  // lhs.local_positions <= rhs.local_positions
  // lhs.frames > rhs.frames
  frames = CalleePortFrames(
      /* local_positions */ {},
      {test::make_taint_config(test_kind_one, test::FrameProperties{}),
       test::make_taint_config(test_kind_two, test::FrameProperties{})});
  frames.difference_with(CalleePortFrames(
      LocalPositionSet{test_position_one},
      {test::make_taint_config(test_kind_one, test::FrameProperties{})}));
  EXPECT_EQ(
      frames,
      CalleePortFrames(
          /* local_positions */ {},
          {test::make_taint_config(test_kind_two, test::FrameProperties{})}));

  // lhs.local_positions > rhs.local_positions
  // lhs.frames > rhs.frames
  frames = CalleePortFrames(
      LocalPositionSet{test_position_one},
      {test::make_taint_config(test_kind_one, test::FrameProperties{}),
       test::make_taint_config(test_kind_two, test::FrameProperties{})});
  frames.difference_with(CalleePortFrames(
      /* local_positions */ {},
      {test::make_taint_config(test_kind_one, test::FrameProperties{})}));
  EXPECT_EQ(
      frames,
      CalleePortFrames(
          LocalPositionSet{test_position_one},
          {test::make_taint_config(test_kind_one, test::FrameProperties{}),
           test::make_taint_config(test_kind_two, test::FrameProperties{})}));

  // lhs.local_positions > rhs.local_positions
  // lhs.frames <= rhs.frames
  frames = CalleePortFrames(
      LocalPositionSet{test_position_one},
      {test::make_taint_config(test_kind_one, test::FrameProperties{})});
  frames.difference_with(CalleePortFrames(
      /* local_positions */ {},
      {test::make_taint_config(test_kind_one, test::FrameProperties{}),
       test::make_taint_config(test_kind_two, test::FrameProperties{})}));
  EXPECT_EQ(
      frames,
      CalleePortFrames(
          LocalPositionSet{test_position_one},
          {test::make_taint_config(test_kind_one, test::FrameProperties{})}));
}

TEST_F(CalleePortFramesTest, Iterator) {
  auto context = test::make_empty_context();

  auto* test_kind_one = context.kinds->get("TestSinkOne");
  auto* test_kind_two = context.kinds->get("TestSinkTwo");

  auto callee_port_frames = CalleePortFrames(
      /* local_positions */ {},
      {test::make_taint_config(test_kind_one, test::FrameProperties{}),
       test::make_taint_config(test_kind_two, test::FrameProperties{})});

  std::vector<Frame> frames;
  for (const auto& frame : callee_port_frames) {
    frames.push_back(frame);
  }

  EXPECT_EQ(frames.size(), 2);
  EXPECT_NE(
      std::find(
          frames.begin(),
          frames.end(),
          test::make_taint_frame(test_kind_one, test::FrameProperties{})),
      frames.end());
  EXPECT_NE(
      std::find(
          frames.begin(),
          frames.end(),
          test::make_taint_frame(test_kind_two, test::FrameProperties{})),
      frames.end());
}

TEST_F(CalleePortFramesTest, Map) {
  auto context = test::make_empty_context();

  Scope scope;
  auto* one =
      context.methods->create(redex::create_void_method(scope, "LOne;", "one"));
  auto* test_kind_one = context.kinds->get("TestSinkOne");
  auto* test_kind_two = context.kinds->get("TestSinkTwo");
  auto* feature_one = context.features->get("FeatureOne");

  auto frames = CalleePortFrames(
      /* local_positions */ {},
      {
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = one,
                  .distance = 1,
                  .origins = MethodSet{one}}),
          test::make_taint_config(
              test_kind_two,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                  .callee = one,
                  .distance = 2,
                  .origins = MethodSet{one}}),
      });
  frames.map([feature_one](Frame& frame) {
    frame.add_inferred_features(FeatureMayAlwaysSet{feature_one});
  });
  EXPECT_EQ(
      frames,
      (CalleePortFrames(
          /* local_positions */ {},
          {
              test::make_taint_config(
                  test_kind_one,
                  test::FrameProperties{
                      .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                      .callee = one,
                      .distance = 1,
                      .origins = MethodSet{one},
                      .locally_inferred_features =
                          FeatureMayAlwaysSet{feature_one}}),
              test::make_taint_config(
                  test_kind_two,
                  test::FrameProperties{
                      .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                      .callee = one,
                      .distance = 2,
                      .origins = MethodSet{one},
                      .locally_inferred_features =
                          FeatureMayAlwaysSet{feature_one}}),
          })));
}

TEST_F(CalleePortFramesTest, FeaturesAndPositions) {
  auto context = test::make_empty_context();

  Scope scope;
  auto* test_kind_one = context.kinds->get("TestSinkOne");
  auto* test_kind_two = context.kinds->get("TestSinkTwo");
  auto* test_position_one = context.positions->get(std::nullopt, 1);
  auto* test_position_two = context.positions->get(std::nullopt, 2);
  auto* feature_one = context.features->get("FeatureOne");
  auto* feature_two = context.features->get("FeatureTwo");

  // add_inferred_features should be an *add* operation on the features,
  // not a join.
  auto frames = CalleePortFrames(
      /* local_positions */ {},
      {test::make_taint_config(
          test_kind_one,
          test::FrameProperties{
              .locally_inferred_features = FeatureMayAlwaysSet(
                  /* may */ FeatureSet{feature_one},
                  /* always */ FeatureSet{})})});
  frames.add_inferred_features(FeatureMayAlwaysSet{feature_two});
  EXPECT_EQ(
      frames.inferred_features(),
      FeatureMayAlwaysSet(
          /* may */ FeatureSet{feature_one},
          /* always */ FeatureSet{feature_two}));

  frames = CalleePortFrames(
      /* local_positions */ LocalPositionSet{test_position_one},
      {
          test::make_taint_config(test_kind_one, test::FrameProperties{}),
          test::make_taint_config(test_kind_two, test::FrameProperties{}),
      });
  EXPECT_EQ(frames.local_positions(), (LocalPositionSet{test_position_one}));

  frames.add_local_position(test_position_two);
  EXPECT_EQ(
      frames.local_positions(),
      (LocalPositionSet{test_position_one, test_position_two}));

  frames.set_local_positions(LocalPositionSet{test_position_two});
  EXPECT_EQ(frames.local_positions(), (LocalPositionSet{test_position_two}));

  frames.add_inferred_features_and_local_position(
      /* features */ FeatureMayAlwaysSet{feature_one},
      /* position */ test_position_one);
  EXPECT_EQ(frames.inferred_features(), FeatureMayAlwaysSet{feature_one});
  EXPECT_EQ(
      frames.local_positions(),
      (LocalPositionSet{test_position_one, test_position_two}));
}

TEST_F(CalleePortFramesTest, Propagate) {
  auto context = test::make_empty_context();

  Scope scope;
  auto* one =
      context.methods->create(redex::create_void_method(scope, "LOne;", "one"));
  auto* two =
      context.methods->create(redex::create_void_method(scope, "LTwo;", "two"));

  auto* test_kind_one = context.kinds->get("TestSinkOne");
  auto* test_kind_two = context.kinds->get("TestSinkTwo");
  auto* call_position = context.positions->get("Test.java", 1);

  // Test propagating non-crtex frames. Crtex-ness determined by callee port.
  auto non_crtex_frames = CalleePortFrames(
      /* local_positions */ {},
      {
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee = one, .distance = 1, .origins = MethodSet{one}}),
          test::make_taint_config(
              test_kind_two,
              test::FrameProperties{.callee = one, .origins = MethodSet{one}}),
      });
  EXPECT_EQ(
      non_crtex_frames.propagate(
          /* callee */ two,
          /* callee_port */ AccessPath(Root(Root::Kind::Argument, 0)),
          call_position,
          /* maximum_source_sink_distance */ 100,
          context,
          /* source_register_types */ {},
          /* source_constant_arguments */ {}),
      (CalleePortFrames(
          /* local_positions */ {},
          {
              test::make_taint_config(
                  test_kind_one,
                  test::FrameProperties{
                      .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                      .callee = two,
                      .call_position = call_position,
                      .distance = 2,
                      .origins = MethodSet{one},
                      .locally_inferred_features =
                          FeatureMayAlwaysSet::bottom()}),
              test::make_taint_config(
                  test_kind_two,
                  test::FrameProperties{
                      .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                      .callee = two,
                      .call_position = call_position,
                      .distance = 1,
                      .origins = MethodSet{one},
                      .locally_inferred_features =
                          FeatureMayAlwaysSet::bottom()}),
          })));

  // Test propagating crtex frames (callee port == anchor).
  auto crtex_frames = CalleePortFrames(
      /* local_positions */ {},
      {
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Anchor)),
                  .origins = MethodSet{one},
                  .canonical_names =
                      CanonicalNameSetAbstractDomain{
                          CanonicalName(CanonicalName::TemplateValue{
                              "%programmatic_leaf_name%"})}}),
          test::make_taint_config(
              test_kind_two,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Anchor)),
                  .origins = MethodSet{one},
                  .canonical_names =
                      CanonicalNameSetAbstractDomain{CanonicalName(
                          CanonicalName::TemplateValue{"constant value"})}}),
      });

  auto expected_instantiated_name =
      CanonicalName(CanonicalName::InstantiatedValue{two->signature()});
  auto propagated_crtex_frames = crtex_frames.propagate(
      /* callee */ two,
      /* callee_port */ AccessPath(Root(Root::Kind::Argument, 0)),
      call_position,
      /* maximum_source_sink_distance */ 100,
      context,
      /* source_register_types */ {},
      /* source_constant_arguments */ {});
  EXPECT_EQ(
      propagated_crtex_frames,
      (CalleePortFrames(
          /* local_positions */ {},
          {
              test::make_taint_config(
                  test_kind_one,
                  test::FrameProperties{
                      .callee_port = AccessPath(
                          Root(Root::Kind::Anchor),
                          Path{DexString::make_string("Argument(-1)")}),
                      .callee = two,
                      .call_position = call_position,
                      .origins = MethodSet{one},
                      .locally_inferred_features =
                          FeatureMayAlwaysSet::bottom(),
                      .canonical_names =
                          CanonicalNameSetAbstractDomain{
                              expected_instantiated_name}}),
              test::make_taint_config(
                  test_kind_two,
                  test::FrameProperties{
                      .callee_port = AccessPath(
                          Root(Root::Kind::Anchor),
                          Path{DexString::make_string("Argument(-1)")}),
                      .callee = two,
                      .call_position = call_position,
                      .origins = MethodSet{one},
                      .locally_inferred_features =
                          FeatureMayAlwaysSet::bottom(),
                      .canonical_names = CanonicalNameSetAbstractDomain(
                          CanonicalName(CanonicalName::InstantiatedValue{
                              "constant value"}))}),
          })));

  // Test propagating crtex-like frames (callee port == anchor.<path>),
  // specifically, propagate the propagated frames above again. These frames
  // originate from crtex leaves, but are not themselves the leaves.
  EXPECT_EQ(
      propagated_crtex_frames.propagate(
          /* callee */ two,
          /* callee_port */ AccessPath(Root(Root::Kind::Argument, 0)),
          call_position,
          /* maximum_source_sink_distance */ 100,
          context,
          /* source_register_types */ {},
          /* source_constant_arguments */ {}),
      (CalleePortFrames(
          /* local_positions */ {},
          {
              test::make_taint_config(
                  test_kind_one,
                  test::FrameProperties{
                      .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                      .callee = two,
                      .call_position = call_position,
                      .distance = 1,
                      .origins = MethodSet{one},
                      .locally_inferred_features =
                          FeatureMayAlwaysSet::bottom(),
                  }),
              test::make_taint_config(
                  test_kind_two,
                  test::FrameProperties{
                      .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                      .callee = two,
                      .call_position = call_position,
                      .distance = 1,
                      .origins = MethodSet{one},
                      .locally_inferred_features =
                          FeatureMayAlwaysSet::bottom(),
                  }),
          })));
}

TEST_F(CalleePortFramesTest, PropagateDropFrames) {
  auto context = test::make_empty_context();

  Scope scope;
  auto* one =
      context.methods->create(redex::create_void_method(scope, "LOne;", "one"));
  auto* two =
      context.methods->create(redex::create_void_method(scope, "LTwo;", "two"));

  auto* test_kind_one = context.kinds->get("TestSinkOne");
  auto* test_kind_two = context.kinds->get("TestSinkTwo");
  auto* call_position = context.positions->get("Test.java", 1);
  auto* user_feature_one = context.features->get("UserFeatureOne");
  auto* user_feature_two = context.features->get("UserFeatureTwo");

  // Propagating this frame will give it a distance of 2. It is expected to be
  // dropped as it exceeds the maximum distance allowed.
  auto frames = CalleePortFrames(
      /* local_positions */ {},
      {
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{.callee = one, .distance = 1}),
      });
  EXPECT_EQ(
      frames.propagate(
          /* callee */ two,
          /* callee_port */ AccessPath(Root(Root::Kind::Argument, 0)),
          call_position,
          /* maximum_source_sink_distance */ 1,
          context,
          /* source_register_types */ {},
          /* source_constant_arguments */ {}),
      CalleePortFrames::bottom());

  // One of the two frames will be ignored during propagation because its
  // distance exceeds the maximum distance allowed.
  frames = CalleePortFrames(
      /* local_positions */ {},
      {
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee = one,
                  .distance = 2,
                  .user_features = FeatureSet{user_feature_one}}),
          test::make_taint_config(
              test_kind_two,
              test::FrameProperties{
                  .callee = one,
                  .distance = 1,
                  .user_features = FeatureSet{user_feature_two}}),
      });
  EXPECT_EQ(
      frames.propagate(
          /* callee */ two,
          /* callee_port */ AccessPath(Root(Root::Kind::Argument, 0)),
          call_position,
          /* maximum_source_sink_distance */ 2,
          context,
          /* source_register_types */ {},
          /* source_constant_arguments */ {}),
      (CalleePortFrames(
          /* local_positions */ {},
          {
              test::make_taint_config(
                  test_kind_two,
                  test::FrameProperties{
                      .callee_port = AccessPath(Root(Root::Kind::Argument, 0)),
                      .callee = two,
                      .call_position = call_position,
                      .distance = 2,
                      .inferred_features =
                          FeatureMayAlwaysSet{user_feature_two},
                      .locally_inferred_features =
                          FeatureMayAlwaysSet::bottom()}),
          })));
}

TEST_F(CalleePortFramesTest, TransformKindWithFeatures) {
  auto context = test::make_empty_context();

  auto* feature_one = context.features->get("FeatureOne");
  auto* feature_two = context.features->get("FeatureTwo");
  auto* user_feature_one = context.features->get("UserFeatureOne");

  auto* test_kind_one = context.kinds->get("TestKindOne");
  auto* test_kind_two = context.kinds->get("TestKindTwo");
  auto* transformed_test_kind_one =
      context.kinds->get("TransformedTestKindOne");
  auto* transformed_test_kind_two =
      context.kinds->get("TransformedTestKindTwo");

  auto initial_frames = CalleePortFrames(
      /* local_positions */ {},
      {
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .user_features = FeatureSet{user_feature_one}}),
          test::make_taint_config(
              test_kind_two,
              test::FrameProperties{
                  .inferred_features = FeatureMayAlwaysSet{feature_one},
                  .user_features = FeatureSet{user_feature_one}}),
      });

  // Drop all kinds.
  auto frames = initial_frames;
  frames.transform_kind_with_features(
      [](const auto* /* unused kind */) { return std::vector<const Kind*>(); },
      [](const auto* /* unused kind */) {
        return FeatureMayAlwaysSet::bottom();
      });
  EXPECT_EQ(frames, CalleePortFrames::bottom());

  // Perform an actual transformation.
  frames = initial_frames;
  frames.transform_kind_with_features(
      [&](const auto* kind) -> std::vector<const Kind*> {
        if (kind == test_kind_one) {
          return {transformed_test_kind_one};
        }
        return {kind};
      },
      [](const auto* /* unused kind */) {
        return FeatureMayAlwaysSet::bottom();
      });
  EXPECT_EQ(
      frames,
      (CalleePortFrames(
          /* local_positions */ {},
          {
              test::make_taint_config(
                  transformed_test_kind_one,
                  test::FrameProperties{
                      .user_features = FeatureSet{user_feature_one}}),
              test::make_taint_config(
                  test_kind_two,
                  test::FrameProperties{
                      .inferred_features = FeatureMayAlwaysSet{feature_one},
                      .user_features = FeatureSet{user_feature_one}}),
          })));

  // Another transformation, this time including a change to the features
  frames = initial_frames;
  frames.transform_kind_with_features(
      [&](const auto* kind) {
        if (kind == test_kind_one) {
          return std::vector<const Kind*>{transformed_test_kind_one};
        }
        return std::vector<const Kind*>{kind};
      },
      [feature_one](const auto* /* unused kind */) {
        return FeatureMayAlwaysSet{feature_one};
      });
  EXPECT_EQ(
      frames,
      (CalleePortFrames(
          /* local_positions */ {},
          {
              test::make_taint_config(
                  transformed_test_kind_one,
                  test::FrameProperties{
                      .locally_inferred_features =
                          FeatureMayAlwaysSet{feature_one},
                      .user_features = FeatureSet{user_feature_one}}),
              test::make_taint_config(
                  test_kind_two,
                  test::FrameProperties{
                      .inferred_features = FeatureMayAlwaysSet{feature_one},
                      .user_features = FeatureSet{user_feature_one}}),
          })));

  // Tests one -> many transformations (with features).
  frames = initial_frames;
  frames.transform_kind_with_features(
      [&](const auto* kind) {
        if (kind == test_kind_one) {
          return std::vector<const Kind*>{
              test_kind_one,
              transformed_test_kind_one,
              transformed_test_kind_two};
        }
        return std::vector<const Kind*>{};
      },
      [feature_one](const auto* /* unused kind */) {
        return FeatureMayAlwaysSet{feature_one};
      });
  EXPECT_EQ(
      frames,
      (CalleePortFrames(
          /* local_positions */ {},
          {
              test::make_taint_config(
                  test_kind_one,
                  test::FrameProperties{
                      .locally_inferred_features =
                          FeatureMayAlwaysSet{feature_one},
                      .user_features = FeatureSet{user_feature_one}}),
              test::make_taint_config(
                  transformed_test_kind_one,
                  test::FrameProperties{
                      .locally_inferred_features =
                          FeatureMayAlwaysSet{feature_one},
                      .user_features = FeatureSet{user_feature_one}}),
              test::make_taint_config(
                  transformed_test_kind_two,
                  test::FrameProperties{
                      .locally_inferred_features =
                          FeatureMayAlwaysSet{feature_one},
                      .user_features = FeatureSet{user_feature_one}}),
          })));

  // Tests transformations with features added to specific kinds.
  frames = initial_frames;
  frames.transform_kind_with_features(
      [&](const auto* kind) {
        if (kind == test_kind_one) {
          return std::vector<const Kind*>{
              transformed_test_kind_one, transformed_test_kind_two};
        }
        return std::vector<const Kind*>{};
      },
      [&](const auto* transformed_kind) {
        if (transformed_kind == transformed_test_kind_one) {
          return FeatureMayAlwaysSet{feature_one};
        }
        return FeatureMayAlwaysSet::bottom();
      });
  EXPECT_EQ(
      frames,
      (CalleePortFrames(
          /* local_positions */ {},
          {
              test::make_taint_config(
                  transformed_test_kind_one,
                  test::FrameProperties{
                      .locally_inferred_features =
                          FeatureMayAlwaysSet{feature_one},
                      .user_features = FeatureSet{user_feature_one}}),
              test::make_taint_config(
                  transformed_test_kind_two,
                  test::FrameProperties{
                      .user_features = FeatureSet{user_feature_one}}),
          })));

  // Transformation where multiple old kinds map to the same new kind
  frames = CalleePortFrames(
      /* local_positions */ {},
      {
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .inferred_features = FeatureMayAlwaysSet{feature_two},
                  .user_features = FeatureSet{user_feature_one}}),
          test::make_taint_config(
              test_kind_two,
              test::FrameProperties{
                  .inferred_features = FeatureMayAlwaysSet{feature_one},
                  .user_features = FeatureSet{user_feature_one}}),
      });
  frames.transform_kind_with_features(
      [&](const auto* /* unused kind */) -> std::vector<const Kind*> {
        return {transformed_test_kind_one};
      },
      [](const auto* /* unused kind */) {
        return FeatureMayAlwaysSet::bottom();
      });
  EXPECT_EQ(
      frames,
      (CalleePortFrames(
          /* local_positions */ {},
          {
              test::make_taint_config(
                  transformed_test_kind_one,
                  test::FrameProperties{
                      .inferred_features = FeatureMayAlwaysSet(
                          /* may */ FeatureSet{feature_one, feature_two},
                          /* always */ FeatureSet{}),
                      .user_features = FeatureSet{user_feature_one}}),
          })));
}

TEST_F(CalleePortFramesTest, AppendCalleePort) {
  auto context = test::make_empty_context();

  const auto* path_element1 = DexString::make_string("field1");
  const auto* path_element2 = DexString::make_string("field2");

  auto frames = CalleePortFrames(
      /* local_positions */ {},
      {test::make_taint_config(
          Kinds::artificial_source(),
          test::FrameProperties{
              .callee_port = AccessPath(
                  Root(Root::Kind::Argument), Path{path_element1})})});

  frames.append_callee_port_to_artificial_sources(path_element2);
  EXPECT_EQ(
      frames,
      (CalleePortFrames(
          /* local_positions */ {},
          {test::make_taint_config(
              Kinds::artificial_source(),
              test::FrameProperties{
                  .callee_port = AccessPath(
                      Root(Root::Kind::Argument),
                      Path{path_element1, path_element2})})})));
}

TEST_F(CalleePortFramesTest, FilterInvalidFrames) {
  auto context = test::make_empty_context();

  Scope scope;
  auto* method1 =
      context.methods->create(redex::create_void_method(scope, "LOne;", "one"));
  auto* test_kind_one = context.kinds->get("TestSourceOne");
  auto* test_kind_two = context.kinds->get("TestSourceTwo");

  // Filter by callee. In practice, this scenario where the frames each contain
  // a different callee will not happen. These frames will be never show up in
  // the same `CalleePortFrames` object.
  //
  // TODO(T91357916): Move callee, call_position and callee_port out of `Frame`
  // and re-visit these tests. Signature of `filter_invalid_frames` will likely
  // change.
  auto frames = CalleePortFrames(
      /* local_positions */ {},
      {test::make_taint_config(
           test_kind_one,
           test::FrameProperties{
               .callee_port = AccessPath(Root(Root::Kind::Argument))}),
       test::make_taint_config(
           test_kind_two,
           test::FrameProperties{
               .callee_port = AccessPath(Root(Root::Kind::Argument)),
               .callee = method1})});
  frames.filter_invalid_frames(
      /* is_valid */
      [&](const Method* MT_NULLABLE callee,
          const AccessPath& /* callee_port */,
          const Kind* /* kind */) { return callee == nullptr; });
  EXPECT_EQ(
      frames,
      (CalleePortFrames(
          /* local_positions */ {},
          {test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument))})})));

  // Filter by callee port (drops nothing)
  frames = CalleePortFrames(
      /* local_positions */ {},
      {test::make_taint_config(
          Kinds::artificial_source(),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument)),
              .callee = method1})});
  frames.filter_invalid_frames(
      /* is_valid */
      [&](const Method* MT_NULLABLE /* callee */,
          const AccessPath& callee_port,
          const Kind* /* kind */) {
        return callee_port == AccessPath(Root(Root::Kind::Argument));
      });
  EXPECT_EQ(
      frames,
      (CalleePortFrames(
          /* local_positions */ {},
          {test::make_taint_config(
              Kinds::artificial_source(),
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument)),
                  .callee = method1})})));

  // Filter by callee port (drops everything)
  frames = CalleePortFrames(
      /* local_positions */ {},
      {test::make_taint_config(
          Kinds::artificial_source(),
          test::FrameProperties{
              .callee_port = AccessPath(Root(Root::Kind::Argument)),
              .callee = method1})});
  frames.filter_invalid_frames(
      /* is_valid */
      [&](const Method* MT_NULLABLE /* callee */,
          const AccessPath& callee_port,
          const Kind* /* kind */) {
        return callee_port != AccessPath(Root(Root::Kind::Argument));
      });
  EXPECT_EQ(frames, CalleePortFrames::bottom());

  // Filter by kind
  frames = CalleePortFrames(
      /* local_positions */ {},
      {test::make_taint_config(
           test_kind_one,
           test::FrameProperties{
               .callee_port = AccessPath(Root(Root::Kind::Argument))}),
       test::make_taint_config(
           test_kind_two,
           test::FrameProperties{
               .callee_port = AccessPath(Root(Root::Kind::Argument)),
               .callee = method1})});
  frames.filter_invalid_frames(
      /* is_valid */
      [&](const Method* MT_NULLABLE /* callee */,
          const AccessPath& /* callee_port */,
          const Kind* kind) { return kind != test_kind_two; });
  EXPECT_EQ(
      frames,
      (CalleePortFrames(
          /* local_positions */ {},
          {test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Argument))})})));
}

TEST_F(CalleePortFramesTest, Show) {
  auto context = test::make_empty_context();

  Scope scope;
  auto* one =
      context.methods->create(redex::create_void_method(scope, "LOne;", "one"));
  auto* test_kind_one = context.kinds->get("TestSink1");
  auto frame_one = test::make_taint_config(
      test_kind_one, test::FrameProperties{.origins = MethodSet{one}});
  auto frames = CalleePortFrames(/* local_positions */ {}, {frame_one});

  EXPECT_EQ(
      show(frames),
      "CalleePortFrames(callee_port=AccessPath(Leaf), "
      "is_artificial_source_frames=0, frames=[FrameByKind(kind=TestSink1, "
      "frames={Frame(kind=`TestSink1`, callee_port=AccessPath(Leaf), "
      "origins={`LOne;.one:()V`})}),])");

  frames = CalleePortFrames(
      LocalPositionSet{context.positions->get(std::nullopt, 1)}, {frame_one});
  EXPECT_EQ(
      show(frames),
      "CalleePortFrames(callee_port=AccessPath(Leaf), "
      "is_artificial_source_frames=0, "
      "local_positions={Position(line=1)}, frames=[FrameByKind(kind=TestSink1, "
      "frames={Frame(kind=`TestSink1`, callee_port=AccessPath(Leaf), "
      "origins={`LOne;.one:()V`})}),])");

  EXPECT_EQ(
      show(CalleePortFrames::bottom()),
      "CalleePortFrames(callee_port=AccessPath(Leaf), is_artificial_source_frames=0, frames=[])");

  EXPECT_EQ(show(CalleePortFrames::top()), "T");
}

TEST_F(CalleePortFramesTest, ContainsKind) {
  auto context = test::make_empty_context();

  auto frames = CalleePortFrames(
      /* local_positions */ {},
      {test::make_taint_config(
           /* kind */ context.kinds->get("TestSourceOne"),
           test::FrameProperties{}),
       test::make_taint_config(
           /* kind */ context.kinds->get("TestSourceTwo"),
           test::FrameProperties{})});

  EXPECT_TRUE(frames.contains_kind(context.kinds->get("TestSourceOne")));
  EXPECT_TRUE(frames.contains_kind(context.kinds->get("TestSourceTwo")));
  EXPECT_FALSE(frames.contains_kind(context.kinds->get("TestSink")));
}

TEST_F(CalleePortFramesTest, PartitionByKind) {
  auto context = test::make_empty_context();

  auto* test_kind_one = context.kinds->get("TestSource1");
  auto* test_kind_two = context.kinds->get("TestSource2");

  auto frames = CalleePortFrames(
      /* local_positions */ {},
      {
          test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Return))}),
          test::make_taint_config(
              test_kind_two,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Return))}),
      });

  auto frames_by_kind = frames.partition_by_kind<const Kind*>(
      [](const Kind* kind) { return kind; });
  EXPECT_TRUE(frames_by_kind.size() == 2);
  EXPECT_EQ(
      frames_by_kind[test_kind_one],
      CalleePortFrames(
          /* local_positions */ {},
          {test::make_taint_config(
              test_kind_one,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Return))})}));
  EXPECT_EQ(
      frames_by_kind[test_kind_one].callee_port(),
      AccessPath(Root(Root::Kind::Return)));
  EXPECT_EQ(
      frames_by_kind[test_kind_two],
      CalleePortFrames(
          /* local_positions */ {},
          {test::make_taint_config(
              test_kind_two,
              test::FrameProperties{
                  .callee_port = AccessPath(Root(Root::Kind::Return))})}));
  EXPECT_EQ(
      frames_by_kind[test_kind_two].callee_port(),
      AccessPath(Root(Root::Kind::Return)));
}

} // namespace marianatrench
